/*
 * Asp engine string implementation.
 */

#include "asp.h"
#include "range.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if !defined ASP_ENGINE_VERSION_MAJOR || \
    !defined ASP_ENGINE_VERSION_MINOR || \
    !defined ASP_ENGINE_VERSION_PATCH || \
    !defined ASP_ENGINE_VERSION_TWEAK
#error ASP_ENGINE_VERSION_* macros undefined
#endif

static AspDataEntry *ToString
    (AspEngine *, const AspDataEntry *entry, bool repr);
static const char *TypeString(DataType);

/* If supplied, *size is the number of bytes in the string without null
   termination. If *size - index < bufferSize, this routine will deposit a
   null after the requested string content. If *size - index >= bufferSize,
   no null termination occurs. Negative indices are not supported. */
bool AspStringValue
    (AspEngine *engine, const AspDataEntry *entry,
     size_t *size, char *buffer, size_t index, size_t bufferSize)
{
    if (!AspIsString(entry))
        return false;

    size_t localSize = (size_t)AspDataGetSequenceCount(entry);
    if (size != 0)
        *size = localSize;

    if (buffer != 0 && bufferSize != 0)
    {
        if (localSize > bufferSize)
            localSize = bufferSize;

        uint32_t iterationCount = 0;
        for (AspSequenceResult fragmentResult =
             AspSequenceNext(engine, entry, 0, true);
             iterationCount < engine->cycleDetectionLimit &&
             localSize > 0 && fragmentResult.element != 0;
             iterationCount++,
             fragmentResult = AspSequenceNext
                (engine, entry, fragmentResult.element, true))
        {
            AspDataEntry *fragment = fragmentResult.value;

            size_t fragmentSize = (size_t)
                AspDataGetStringFragmentSize(fragment);

            if (index >= fragmentSize)
            {
                index -= fragmentSize;
                continue;
            }

            size_t fetchSize = fragmentSize - index;
            if (fetchSize > localSize)
                fetchSize = localSize;

            memcpy
                (buffer, AspDataGetStringFragmentData(fragment) + index,
                 fetchSize);

            index = 0;
            buffer += fetchSize;
            localSize -= fetchSize;
            bufferSize -= fetchSize;
        }
        if (iterationCount >= engine->cycleDetectionLimit)
        {
            engine->runResult = AspRunResult_CycleDetected;
            return false;
        }

        if (bufferSize > 0)
            *buffer = '\0';
    }

    return true;
}

AspDataEntry *AspToString(AspEngine *engine, AspDataEntry *entry)
{
    if (AspIsString(entry))
    {
        AspRef(engine, entry);
        return entry;
    }

    return ToString(engine, entry, false);
}

AspDataEntry *AspToRepr(AspEngine *engine, const AspDataEntry *entry)
{
    return ToString(engine, entry, true);
}

static AspDataEntry *ToString
    (AspEngine *engine, const AspDataEntry *entry, bool repr)
{
    AspDataEntry *result = AspNewString(engine, 0, 0);
    if (result == 0)
        return 0;

    /* Avoid recursion by using the engine's stack. */
    const AspDataEntry *startStackTop = engine->stackTop;
    const AspDataEntry *next = 0;
    bool flag = false;
    uint32_t state = 0;
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        char buffer[100];
        DataType type = (DataType)AspDataGetType(entry);
        switch (type)
        {
            default:
                strcpy(buffer, "?");
                break;

            case DataType_None:
                strcpy(buffer, "None");
                break;

            case DataType_Ellipsis:
                strcpy(buffer, "...");
                break;

            case DataType_Boolean:
                strcpy(buffer, AspIsTrue(engine, entry) ? "True" : "False");
                break;

            case DataType_Integer:
            {
                int32_t i;
                AspIntegerValue(entry, &i);
                snprintf(buffer, sizeof buffer, "%d", (int)i);
                break;
            }

            case DataType_Float:
            {
                int count = 0;
                double f;
                AspFloatValue(entry, &f);
                count += snprintf
                    (buffer + count, sizeof buffer - count, "%g", f);
                if (!isnan(f) && !isinf(f) &&
                    strchr(buffer, '.') == 0 && strchr(buffer, 'e') == 0)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, ".0");
                break;
            }

            case DataType_Symbol:
            {
                int32_t symbol;
                AspSymbolValue(entry, &symbol);
                snprintf(buffer, sizeof buffer, "`%d", symbol);
                break;
            }

            case DataType_Range:
            {
                int count = 0;
                int32_t start, end, step;
                bool bounded;
                AspGetRange(engine, entry, &start, &end, &step, &bounded);
                if (start != (step < 0 ? -1 : 0))
                    count += snprintf
                        (buffer + count, sizeof buffer - count, "%d", start);
                count += snprintf
                    (buffer + count, sizeof buffer - count, "..");
                if (bounded)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, "%d", end);
                if (step != 1)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, ":%d", step);
                break;
            }

            case DataType_String:
            {
                /* Instead of using the intermediate buffer, append directly
                   to the resulting string. */
                *buffer = '\0';

                /* Wrap the string with quotes if applicable. */
                if (repr || startStackTop != engine->stackTop)
                {
                    AspRunResult appendResult = AspStringAppendBuffer
                        (engine, result, "'", 1);
                    if (appendResult != AspRunResult_OK)
                    {
                        AspUnref(engine, result);
                        result = 0;
                        break;
                    }
                }

                /* Append the string. */
                uint32_t iterationCount = 0;
                for (AspSequenceResult nextResult =
                     AspSequenceNext(engine, entry, 0, true);
                     iterationCount < engine->cycleDetectionLimit &&
                     nextResult.element != 0;
                     iterationCount++,
                     nextResult = AspSequenceNext
                        (engine, entry, nextResult.element, true))
                {
                    AspDataEntry *fragment = nextResult.value;
                    uint8_t fragmentSize =
                        AspDataGetStringFragmentSize(fragment);
                    const char *fragmentData =
                        AspDataGetStringFragmentData(fragment);

                    /* Decide how to treat strings. */
                    if (!repr && startStackTop == engine->stackTop)
                    {
                        /* Copy the string as-is. */
                        AspRunResult appendResult = AspStringAppendBuffer
                            (engine, result, fragmentData, fragmentSize);
                        if (appendResult != AspRunResult_OK)
                        {
                            AspUnref(engine, result);
                            result = 0;
                            break;
                        }
                    }
                    else
                    {
                        /* Encode the string in canonical representation if
                           requested or if it is contained within another
                           structure. */
                        for (uint8_t i = 0; i < fragmentSize; i++)
                        {
                            AspRunResult appendResult = AspRunResult_OK;
                            char c = fragmentData[i];
                            if (isprint(c))
                            {
                                appendResult = AspStringAppendBuffer
                                    (engine, result, &c, 1);
                            }
                            else
                            {
                                char encoded[5] = "\\";
                                char code = 0;
                                switch (c)
                                {
                                    case '\0':
                                        code = '0';
                                        break;
                                    case '\a':
                                        code = 'a';
                                        break;
                                    case '\b':
                                        code = 'b';
                                        break;
                                    case '\f':
                                        code = 'f';
                                        break;
                                    case '\n':
                                        code = 'n';
                                        break;
                                    case '\r':
                                        code = 'r';
                                        break;
                                    case '\t':
                                        code = 't';
                                        break;
                                    case '\v':
                                        code = 'v';
                                        break;
                                    case '\\':
                                        code = '\\';
                                        break;
                                    case '\'':
                                        code = '\'';
                                        break;
                                }
                                if (code != 0)
                                {
                                    encoded[1] = code;
                                    encoded[2] = '\0';
                                }
                                else
                                {
                                    uint8_t uc = *(uint8_t *)&c;
                                    snprintf
                                        (encoded + 1, sizeof encoded - 1,
                                         "x%02x", uc);
                                }
                                appendResult = AspStringAppendBuffer
                                    (engine, result, encoded, strlen(encoded));
                            }
                            if (appendResult != AspRunResult_OK)
                            {
                                AspUnref(engine, result);
                                result = 0;
                                break;
                            }
                        }
                    }
                    if (result == 0)
                        break;
                }
                if (iterationCount >= engine->cycleDetectionLimit)
                {
                    engine->runResult = AspRunResult_CycleDetected;
                    return 0;
                }
                if (result == 0)
                    break;

                /* Close the quote if applicable. */
                if (repr || startStackTop != engine->stackTop)
                {
                    AspRunResult appendResult = AspStringAppendBuffer
                        (engine, result, "'", 1);
                    if (appendResult != AspRunResult_OK)
                    {
                        AspUnref(engine, result);
                        result = 0;
                        break;
                    }
                }

                break;
            }

            case DataType_Tuple:
            case DataType_List:
            {
                /* Append starting punctuation if applicable. */
                *buffer = '\0';
                bool start = next == 0;
                if (start)
                    strcpy(buffer, type == DataType_Tuple ? "(" : "[");

                /* Examine the next element of the sequence. */
                AspSequenceResult nextResult = AspSequenceNext
                    (engine, entry, next, true);
                next = nextResult.element;

                /* Append any applicable punctuation. */
                if (next == 0)
                {
                    if (type == DataType_Tuple && flag)
                        strcpy(buffer, ",");
                    strcat(buffer, type == DataType_Tuple ? ")" : "]");
                    break;
                }
                else if (!start)
                    strcpy(buffer, ", ");

                /* Save state and defer the element to the next iteration. */
                AspDataEntry *entryStackEntry = AspPushNoUse(engine, entry);
                const AspDataEntry *valueStackEntry = AspPushNoUse
                    (engine, nextResult.value);
                if (entryStackEntry == 0 || valueStackEntry == 0)
                {
                    AspUnref(engine, result);
                    result = 0;
                    break;
                }
                AspDataSetStackEntryHasValue2(entryStackEntry, true);
                AspDataSetStackEntryValue2Index
                    (entryStackEntry, AspIndex(engine, next));
                AspDataSetStackEntryFlag(entryStackEntry, start);

                break;
            }

            case DataType_Set:
            case DataType_Dictionary:
            {
                /* Append starting punctuation if applicable. */
                *buffer = '\0';
                bool start = next == 0 && !flag;
                if (start)
                    strcpy(buffer, "{");

                const AspDataEntry *value = 0;
                if (flag)
                {
                   value = AspValueEntry
                       (engine, AspDataGetTreeNodeValueIndex(next));
                   strcpy(buffer, ": ");
                }
                else
                {
                    /* Examine the next element of the sequence. */
                    AspTreeResult nextResult = AspTreeNext
                        (engine, entry, next, true);
                    next = nextResult.node;
                    value = nextResult.key;

                    /* Append any applicable punctuation. */
                    if (next == 0)
                    {
                        if (start && type == DataType_Dictionary)
                            strcat(buffer, ":");
                        strcat(buffer, "}");
                        break;
                    }
                    else if (!start)
                        strcpy(buffer, ", ");
                }

                /* Save state and defer the key or value to the next iteration
                   as appropriate. */
                AspDataEntry *entryStackEntry = AspPushNoUse(engine, entry);
                const AspDataEntry *valueStackEntry = AspPushNoUse
                    (engine, value);
                if (entryStackEntry == 0 || valueStackEntry == 0)
                {
                    AspUnref(engine, result);
                    result = 0;
                    break;
                }
                AspDataSetStackEntryHasValue2(entryStackEntry, true);
                AspDataSetStackEntryValue2Index
                    (entryStackEntry, AspIndex(engine, next));
                AspDataSetStackEntryFlag
                    (entryStackEntry,
                     type == DataType_Dictionary && !flag);

                break;
            }

            case DataType_Object:
            {
                #ifndef ASP_FEATURE_CLASS

                snprintf
                    (buffer, sizeof buffer, "<%s at 0x%07X>",
                     TypeString(type), AspIndex(engine, entry));

                #else

                int count = 0;

                if (!flag)
                {
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "<%s at 0x%07X",
                         TypeString(type), AspIndex(engine, entry));

                    uint32_t classIndex = AspDataGetObjectClassIndex
                        (entry);
                    if (classIndex == 0)
                        flag = true;
                    else
                    {
                        if (!AspIsFeature(engine, AspFeatureBit_Class))
                        {
                            engine->runResult = AspRunResult_UnexpectedType;
                            break;
                        }

                        count += snprintf
                            (buffer + count, sizeof buffer - count, " of ");

                        AspDataEntry *cls = AspValueEntry
                            (engine, AspDataGetObjectClassIndex(entry));
                        if (AspDataGetType(cls) != DataType_Class)
                        {
                            count += snprintf
                                (buffer + count, sizeof buffer - count, "?");
                            flag = true;
                        }
                        else
                        {
                            /* Save state and defer the class to the next
                               iteration. */
                            AspDataEntry *entryStackEntry = AspPushNoUse
                                (engine, entry);
                            const AspDataEntry *classStackEntry = AspPushNoUse
                                (engine, cls);
                            if (entryStackEntry == 0 || classStackEntry == 0)
                            {
                                AspUnref(engine, result);
                                result = 0;
                                break;
                            }
                            AspDataSetStackEntryFlag(entryStackEntry, true);
                        }
                    }
                }

                if (flag)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, ">");

                #endif

                break;
            }

            #ifdef ASP_FEATURE_CLASS

            case DataType_Class:
            {
                if (!AspIsFeature(engine, AspFeatureBit_Class))
                {
                    engine->runResult = AspRunResult_UnexpectedType;
                    break;
                }

                snprintf
                    (buffer, sizeof buffer, "<%s at 0x%07X>",
                     TypeString(type), AspIndex(engine, entry));

                break;
            }

            case DataType_BoundMethod:
            {
                if (!AspIsFeature(engine, AspFeatureBit_Class))
                {
                    engine->runResult = AspRunResult_UnexpectedType;
                    break;
                }

                if (state == 0)
                    snprintf
                        (buffer, sizeof buffer, "<%s ", TypeString(type));
                else if (state == 1)
                    strcpy(buffer, ".");
                else if (state == 2)
                    strcpy(buffer, " of ");
                else
                {
                    strcpy(buffer, ">");
                    break;
                }

                next = AspValueEntry
                    (engine,
                     state == 0 ?
                     AspDataGetBoundMethodClassIndex(entry) :
                     state == 1 ?
                     AspDataGetBoundMethodFunctionIndex(entry) :
                     AspDataGetBoundMethodInstanceIndex(entry));

                /* Save state and defer the components to the next
                   iteration. */
                AspDataEntry *entryStackEntry = AspPushNoUse(engine, entry);
                const AspDataEntry *valueStackEntry = AspPushNoUse
                    (engine, next);
                if (entryStackEntry == 0 || valueStackEntry == 0)
                {
                    AspUnref(engine, result);
                    result = 0;
                    break;
                }
                AspDataSetStackEntryState(entryStackEntry, state + 1);

                break;
            }

            case DataType_Super:
            {
                if (!AspIsFeature(engine, AspFeatureBit_Class))
                {
                    engine->runResult = AspRunResult_UnexpectedType;
                    break;
                }

                if (state == 0)
                    snprintf
                        (buffer, sizeof buffer, "<%s: ", TypeString(type));
                else if (state == 1)
                     strcpy(buffer, ", ");
                else
                {
                     strcpy(buffer, ">");
                     break;
                }

                next = AspValueEntry
                    (engine,
                     state == 0 ?
                     AspDataGetSuperClassIndex(entry) :
                     AspDataGetSuperInstanceIndex(entry));

                /* Save state and defer the components to the next
                   iteration. */
                AspDataEntry *entryStackEntry = AspPushNoUse(engine, entry);
                const AspDataEntry *valueStackEntry = AspPushNoUse
                    (engine, next);
                if (entryStackEntry == 0 || valueStackEntry == 0)
                {
                    AspUnref(engine, result);
                    result = 0;
                    break;
                }
                AspDataSetStackEntryState(entryStackEntry, state + 1);

                break;
            }

            #endif

            case DataType_Function:
            {
                int count = 0;
                if (AspDataGetFunctionIsApp(entry))
                    count += snprintf
                        (buffer + count, sizeof buffer - count, "<app %s %d",
                         TypeString(type), AspDataGetFunctionSymbol(entry));
                else
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "<%s at 0x%07X, code @0x%07X",
                         TypeString(type), AspIndex(engine, entry),
                         AspDataGetFunctionCodeAddress(entry));
                count += snprintf
                    (buffer + count, sizeof buffer - count, ">");
                break;
            }

            case DataType_Module:
            {
                int count = 0;
                if (AspDataGetModuleIsApp(entry))
                    count += snprintf
                        (buffer + count, sizeof buffer - count, "<app %s %d",
                         TypeString(type), AspDataGetModuleSymbol(entry));
                else
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "<%s at 0x%07X, code @0x%07X",
                         TypeString(type), AspIndex(engine, entry),
                         AspDataGetModuleCodeAddress(entry));
                count += snprintf
                    (buffer + count, sizeof buffer - count, ">");
                break;
            }

            case DataType_ReverseIterator:
            case DataType_ForwardIterator:
            {
                int count = 0;
                uint32_t iterableIndex =
                    AspDataGetIteratorIterableIndex(entry);
                const AspDataEntry *iterable = AspValueEntry
                    (engine, iterableIndex);
                count += snprintf
                    (buffer + count, sizeof buffer - count, "<%s %s",
                     iterable == 0 ? "?" :
                     TypeString(AspDataGetType(iterable)),
                     TypeString(type));
                uint32_t memberIndex = AspDataGetIteratorMemberIndex(entry);
                if (memberIndex == 0)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, " @end");
                count += snprintf
                    (buffer + count, sizeof buffer - count, ">");
                break;
            }

            case DataType_AppIntegerObject:
            case DataType_AppPointerObject:
            {
                int count = 0;
                const AspDataEntry *infoEntry = AspAppObjectInfoEntry
                    (engine, (AspDataEntry *)entry);
                count += snprintf
                    (buffer + count, sizeof buffer - count,
                     "<%s of type %d, value ",
                     TypeString(type), AspDataGetAppObjectType(infoEntry));
                if (infoEntry == 0)
                    count += snprintf
                        (buffer + count, sizeof buffer - count, "?");
                else
                {
                    if (type == DataType_AppIntegerObject)
                    {
                        count += snprintf
                            (buffer + count, sizeof buffer - count, "%d>",
                             AspDataGetAppIntegerObjectValue(infoEntry));
                    }
                    else
                    {
                        count += snprintf
                            (buffer + count, sizeof buffer - count, "%p>",
                             AspDataGetAppPointerObjectValue(infoEntry));
                    }
                }
                break;
            }

            case DataType_Type:
                snprintf
                    (buffer, sizeof buffer, "<%s '%s'>",
                     TypeString(type), TypeString(AspDataGetTypeValue(entry)));
                break;
        }

        /* Check for error. */
        if (result == 0 || engine->runResult != AspRunResult_OK)
            break;

        AspRunResult appendResult = AspStringAppendBuffer
           (engine, result, buffer, strlen(buffer));
        if (appendResult != AspRunResult_OK)
        {
            AspUnref(engine, result);
            result = 0;
            break;
        }

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop)
            break;

        /* Fetch the next item from the stack. */
        entry = AspTopValue(engine);
        next = AspTopValue2(engine);
        flag = AspDataGetStackEntryFlag(engine->stackTop);
        state = AspDataGetStackEntryState(engine->stackTop);
        AspPopNoErase(engine);
    }
    if (iterationCount >= engine->cycleDetectionLimit)
    {
        engine->runResult = AspRunResult_CycleDetected;
        return 0;
    }

    /* Unwind the working stack if necessary. */
    if (engine->runResult == AspRunResult_OK)
    {
        uint32_t iterationCount = 0;
        for (;
             iterationCount < engine->cycleDetectionLimit &&
             engine->stackTop != startStackTop;
             iterationCount++)
        {
            AspPopNoErase(engine);
        }
        if (iterationCount >= engine->cycleDetectionLimit)
        {
            engine->runResult = AspRunResult_CycleDetected;
            return 0;
        }
    }

    return result;
}

static const char *TypeString(DataType type)
{
    switch (type)
    {
        case DataType_None:
            return "None";
        case DataType_Ellipsis:
            return "...";
        case DataType_Boolean:
            return "bool";
        case DataType_Integer:
            return "int";
        case DataType_Float:
            return "float";
        case DataType_Symbol:
            return "symbol";
        case DataType_Range:
            return "range";
        case DataType_String:
            return "str";
        case DataType_Tuple:
            return "tuple";
        case DataType_List:
            return "list";
        case DataType_Set:
            return "set";
        case DataType_Dictionary:
            return "dict";
        case DataType_Object:
            return "object";
        #ifdef ASP_FEATURE_CLASS
        case DataType_Class:
            return "class";
        case DataType_BoundMethod:
            return "bound method";
        case DataType_Super:
            return "super";
        #endif
        case DataType_Function:
            return "function";
        case DataType_Module:
            return "module";
        case DataType_ReverseIterator:
            return "reverse iterator";
        case DataType_ForwardIterator:
            return "iterator";
        case DataType_AppIntegerObject:
            return "app int object";
        case DataType_AppPointerObject:
            return "app pointer object";
        case DataType_Type:
            return "type";
    }

    return "?";
}
