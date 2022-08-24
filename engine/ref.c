/*
 * Asp engine reference counting implementation.
 */

#include "asp.h"
#include "data.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"

static bool IsTerminal(AspDataEntry *);

void AspRef(AspEngine *engine, AspDataEntry *entry)
{
    (void)engine;
    if (AspIsObject(entry))
        AspDataSetUseCount(entry, AspDataGetUseCount(entry) + 1U);
}

void AspUnref(AspEngine *engine, AspDataEntry *entry)
{
    AspDataEntry *startStackTop = engine->stackTop;

    /* Avoid recursion by using Asp's stack. */
    while (true)
    {
        if (AspIsObject(entry))
            AspDataSetUseCount(entry, AspDataGetUseCount(entry) - 1U);
        if (!AspIsObject(entry) || AspDataGetUseCount(entry) == 0)
        {
            uint8_t t = AspDataGetType(entry);
            if (t == DataType_Range)
            {
                if (AspDataGetRangeHasStart(entry))
                    AspUnref(engine, AspValueEntry(engine,
                        AspDataGetRangeStartIndex(entry)));

                if (AspDataGetRangeHasEnd(entry))
                    AspUnref(engine, AspValueEntry(engine,
                        AspDataGetRangeEndIndex(entry)));

                if (AspDataGetRangeHasStep(entry))
                    AspUnref(engine, AspValueEntry(engine,
                        AspDataGetRangeStepIndex(entry)));
            }
            else if (t == DataType_String ||
                     t == DataType_Tuple || t == DataType_List ||
                     t == DataType_ParameterList || t == DataType_ArgumentList)
            {
                AspSequenceResult nextResult = {AspRunResult_OK, 0, 0};
                while ((nextResult = AspSequenceNext
                       (engine, entry, 0)).element != 0)
                {
                    AspRunResult assertResult = AspAssert
                        (engine,
                         AspDataGetType(nextResult.element)
                         == DataType_Element);
                    if (assertResult != AspRunResult_OK)
                        break;

                    bool eraseValue = IsTerminal(nextResult.value);
                    AspSequenceEraseElement
                        (engine, entry, nextResult.element, eraseValue);

                    if (!eraseValue)
                    {
                        if (IsTerminal(nextResult.value))
                            AspUnref(engine, nextResult.value);
                        else
                            AspPushNoUse(engine, nextResult.value);
                    }
                }
            }
            else if (t == DataType_Set || t == DataType_Dictionary ||
                     t == DataType_Namespace)
            {
                AspTreeResult nextResult = {AspRunResult_OK, 0, 0, 0, false};
                while ((nextResult = AspTreeNext(engine, entry, 0)).node != 0)
                {
                    bool eraseKey =
                        nextResult.key != 0 && IsTerminal(nextResult.key);
                    bool eraseValue =
                        nextResult.value != 0 && IsTerminal(nextResult.value);
                    AspTreeEraseNode
                        (engine, entry, nextResult.node,
                         eraseKey, eraseValue);

                    if (nextResult.key != 0 && !eraseKey)
                    {
                        if (IsTerminal(nextResult.key))
                            AspUnref(engine, nextResult.key);
                        else
                            AspPushNoUse(engine, nextResult.key);
                    }
                    if (nextResult.value != 0 && !eraseValue)
                    {
                        if (IsTerminal(nextResult.value))
                            AspUnref(engine, nextResult.value);
                        else
                        {
                            AspDataEntry *entry = AspPushNoUse
                                (engine, nextResult.value);
                            AspAssert(engine, entry != 0);
                        }
                    }
                }
            }
            else if (t == DataType_Iterator)
            {
                AspDataEntry *iterable = AspValueEntry
                    (engine, AspDataGetIteratorIterableIndex(entry));
                AspPushNoUse(engine, iterable);

                AspDataEntry *member = AspEntry
                    (engine, AspDataGetIteratorMemberIndex(entry));
                if (member != 0 && AspDataGetIteratorMemberNeedsCleanup(entry))
                {
                    if (IsTerminal(member))
                        AspUnref(engine, member);
                    else
                        AspPushNoUse(engine, member);
                }
            }
            else if (t == DataType_Function)
            {
                AspDataEntry *module = AspValueEntry
                    (engine, AspDataGetFunctionModuleIndex(entry));
                AspPushNoUse(engine, module);

                AspDataEntry *parameters = AspValueEntry
                    (engine, AspDataGetFunctionParametersIndex(entry));
                AspPushNoUse(engine, parameters);
            }
            else if (t == DataType_Module)
            {
                AspDataEntry *ns = AspValueEntry
                    (engine, AspDataGetModuleNamespaceIndex(entry));
                AspPushNoUse(engine, ns);
            }
            else if (t == DataType_Frame)
            {
                AspDataEntry *module = AspValueEntry
                    (engine, AspDataGetFrameModuleIndex(entry));
                AspPushNoUse(engine, module);
            }
            else if (t == DataType_KeyValuePair)
            {
                AspDataEntry *key = AspValueEntry
                    (engine, AspDataGetKeyValuePairKeyIndex(entry));
                if (IsTerminal(key))
                    AspUnref(engine, key);
                else
                    AspPushNoUse(engine, key);

                AspDataEntry *value = AspValueEntry
                    (engine, AspDataGetKeyValuePairValueIndex(entry));
                if (IsTerminal(value))
                    AspUnref(engine, value);
                else
                    AspPushNoUse(engine, value);
            }
            else if (t == DataType_Parameter)
            {
                if (AspDataGetParameterHasDefault(entry))
                {
                    AspDataEntry *defaultValue = AspValueEntry
                        (engine, AspDataGetParameterDefaultIndex(entry));
                    if (IsTerminal(defaultValue))
                        AspUnref(engine, defaultValue);
                    else
                        AspPushNoUse(engine, defaultValue);
                }
            }
            else if (t == DataType_Argument)
            {
                AspDataEntry *value = AspValueEntry
                    (engine, AspDataGetArgumentValueIndex(entry));
                if (IsTerminal(value))
                    AspUnref(engine, value);
                else
                    AspPushNoUse(engine, value);
            }

            /* Free the entry. */
            if (t != DataType_Free)
                AspFree(engine, AspIndex(engine, entry));
        }

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        entry = AspTop(engine);
        AspPopNoErase(engine);
    }
}

static bool IsTerminal(AspDataEntry *entry)
{
    static uint8_t terminalTypes[] =
    {
        DataType_None,
        DataType_Ellipsis,
        DataType_Boolean,
        DataType_Integer,
        DataType_Float,
        DataType_Type,
        DataType_CodeAddress,
        DataType_StringFragment,
    };

    uint8_t t = AspDataGetType(entry);
    for (unsigned i = 0; i < sizeof terminalTypes/ sizeof *terminalTypes; i++)
        if (t == terminalTypes[i])
            return true;

    return false;
}
