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
    if (engine->runResult != AspRunResult_OK)
        return;

    /* Avoid recursion by using the engine's stack. */
    AspDataEntry *startStackTop = engine->stackTop;
    while (true)
    {
        AspRunResult assertResult = AspAssert(engine, entry != 0);
        if (assertResult != AspRunResult_OK)
            return;
        if (AspIsObject(entry))
            AspDataSetUseCount(entry, AspDataGetUseCount(entry) - 1U);
        if (!AspIsObject(entry) || AspDataGetUseCount(entry) == 0)
        {
            uint8_t t = AspDataGetType(entry);
            if (t == DataType_Boolean)
            {
                AspDataEntry **singleton =
                    AspDataGetBoolean(entry) ?
                    &engine->trueSingleton : &engine->falseSingleton;
                *singleton = 0;
            }
            else if (t == DataType_Range)
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
                    bool eraseSuccess = AspSequenceEraseElement
                        (engine, entry, nextResult.element, eraseValue);
                    if (!eraseSuccess)
                        break;

                    /* Make sure not to free addresses (i.e., elements)
                       within address sequences. */
                    if (!eraseValue &&
                        ((t != DataType_Tuple && t != DataType_List) ||
                         AspIsObject(nextResult.value)))
                        AspPushNoUse(engine, nextResult.value);
                }
            }
            else if (t == DataType_Set || t == DataType_Dictionary ||
                     t == DataType_Namespace)
            {
                AspTreeResult nextResult = {AspRunResult_OK, 0, 0, 0, false};
                while ((nextResult =
                        AspTreeNext(engine, entry, 0, true)).node != 0)
                {
                    bool eraseKey =
                        nextResult.key != 0 && IsTerminal(nextResult.key);
                    bool eraseValue =
                        nextResult.value != 0 &&
                        IsTerminal(nextResult.value) &&
                        AspIsObject(nextResult.value);
                    AspRunResult eraseResult = AspTreeEraseNode
                        (engine, entry, nextResult.node,
                         eraseKey, eraseValue);
                    if (eraseResult != AspRunResult_OK)
                        break;

                    bool pushKey = nextResult.key != 0 && !eraseKey;
                    if (pushKey)
                        AspPushNoUse(engine, nextResult.key);
                    if (nextResult.value != 0 && !eraseValue &&
                        AspIsObject(nextResult.value))
                    {
                        if (pushKey)
                        {
                            AspDataEntry *entry = engine->stackTop;
                            AspDataSetStackEntryHasValue2(entry, true);
                            AspDataSetStackEntryValue2Index
                                (entry, AspIndex(engine, nextResult.value));
                        }
                        else
                        {
                            AspDataEntry *entry = AspPushNoUse
                                (engine, nextResult.value);
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
            else if (t == DataType_AppIntegerObject)
            {
                void (*destructor)(AspEngine *, int16_t, int32_t) =
                    AspDataGetAppIntegerObjectDestructor(entry);
                AspDataEntry *info = AspAppObjectInfoEntry(engine, entry);
                if (destructor != 0 && info != 0)
                {
                    destructor
                        (engine,
                         AspDataGetAppObjectType(info),
                         AspDataGetAppIntegerObjectValue(info));
                }
                if (info != entry)
                    AspUnref(engine, info);
            }
            else if (t == DataType_AppPointerObject)
            {
                void (*destructor)(AspEngine *, int16_t, void *) =
                    AspDataGetAppPointerObjectDestructor(entry);
                AspDataEntry *info = AspAppObjectInfoEntry(engine, entry);
                if (destructor != 0 && info != 0)
                {
                    destructor
                        (engine,
                         AspDataGetAppObjectType(info),
                         AspDataGetAppPointerObjectValue(info));
                }
                if (info != entry)
                    AspUnref(engine, info);
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

        /* Fetch the next item from the stack. */
        entry = AspTopValue2(engine);
        if (entry != 0)
        {
            AspDataSetStackEntryHasValue2(engine->stackTop, false);
            AspDataSetStackEntryValue2Index(engine->stackTop, 0);
        }
        else
        {
            entry = AspTopValue(engine);
            AspPopNoErase(engine);
        }
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
        DataType_AppIntegerObject,
        DataType_AppPointerObject,
        DataType_StringFragment,
    };

    uint8_t t = AspDataGetType(entry);
    for (unsigned i = 0; i < sizeof terminalTypes/ sizeof *terminalTypes; i++)
        if (t == terminalTypes[i])
            return true;

    return false;
}
