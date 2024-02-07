/*
 * Asp API implementation.
 */

#include "asp.h"
#include "range.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"
#include "iterator.h"
#include "symbols.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#if !defined ASP_ENGINE_VERSION_MAJOR || \
    !defined ASP_ENGINE_VERSION_MINOR || \
    !defined ASP_ENGINE_VERSION_PATCH || \
    !defined ASP_ENGINE_VERSION_TWEAK
#error ASP_ENGINE_VERSION_* macros undefined
#endif

static AspDataEntry *ToString
    (AspEngine *, const AspDataEntry *entry, bool repr);
static const char *TypeString(DataType);
static AspDataEntry *NewRange
    (AspEngine *, int32_t start, int32_t *end, int32_t step);
static AspDataEntry *NewObject(AspEngine *, DataType);

void AspEngineVersion(uint8_t version[4])
{
    version[0] = ASP_ENGINE_VERSION_MAJOR;
    version[1] = ASP_ENGINE_VERSION_MINOR;
    version[2] = ASP_ENGINE_VERSION_PATCH;
    version[3] = ASP_ENGINE_VERSION_TWEAK;
}

bool AspIsNone(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_None;
}

bool AspIsEllipsis(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Ellipsis;
}

bool AspIsBoolean(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Boolean;
}

bool AspIsInteger(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Integer;
}

bool AspIsFloat(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Float;
}

bool AspIsIntegral(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_Boolean ||
         type == DataType_Integer);
}

bool AspIsNumber(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_Integer ||
         type == DataType_Float);
}

bool AspIsNumeric(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_Boolean ||
         type == DataType_Integer ||
         type == DataType_Float);
}

bool AspIsRange(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Range;
}

bool AspIsString(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_String;
}

bool AspIsTuple(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Tuple;
}

bool AspIsList(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_List;
}

bool AspIsSequence(const AspDataEntry *entry)
{
    /* In this case, strings are not considered sequences. */
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_Tuple ||
         type == DataType_List);
}

bool AspIsSet(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Set;
}

bool AspIsDictionary(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Dictionary;
}

bool AspIsAppIntegerObject(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_AppIntegerObject;
}

bool AspIsAppPointerObject(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_AppPointerObject;
}

bool AspIsAppObject(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_AppIntegerObject ||
         type == DataType_AppPointerObject);
}

bool AspIsType(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Type;
}

bool AspIsTrue(AspEngine *engine, const AspDataEntry *entry)
{
    switch (AspDataGetType(entry))
    {
        default:
            return AspIsObject(entry);

        case DataType_None:
            return false;

        case DataType_Boolean:
            return AspDataGetBoolean(entry);

        case DataType_Integer:
            return AspDataGetInteger(entry) != 0;

        case DataType_Float:
            return AspDataGetFloat(entry) != 0.0;

        case DataType_Range:
        {
            int32_t startValue, endValue, stepValue;
            AspGetRange(engine, entry, &startValue, &endValue, &stepValue);
            return !AspIsValueAtRangeEnd(startValue, endValue, stepValue);
        }

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
            return AspDataGetSequenceCount(entry) != 0;

        case DataType_Set:
        case DataType_Dictionary:
            return AspDataGetTreeCount(entry) != 0;

        case DataType_Iterator:
            return AspDataGetIteratorMemberIndex(entry) != 0;

        case DataType_Type:
            return AspDataGetTypeValue(entry) != DataType_None;
    }
}

bool AspIntegerValue(const AspDataEntry *entry, int32_t *result)
{
    if (!AspIsNumeric(entry))
        return false;

    int value;
    bool valid = true;
    switch (AspDataGetType(entry))
    {
        default:
            return false;

        case DataType_Boolean:
            value = AspDataGetBoolean(entry) ? 1 : 0;
            break;

        case DataType_Integer:
            value = AspDataGetInteger(entry);
            break;

        case DataType_Float:
        {
            double f = AspDataGetFloat(entry);
            if (isnan(f))
            {
                value = 0;
                valid = false;
            }
            else if (f < INT32_MIN)
            {
                value = INT32_MIN;
                valid = false;
            }
            else if (f > INT32_MAX)
            {
                value = INT32_MAX;
                valid = false;
            }
            else
                value = (int)round(f);
            break;
        }
    }

    if (result != 0)
        *result = value;

    return valid;
}

bool AspFloatValue(const AspDataEntry *entry, double *result)
{
    if (!AspIsNumeric(entry))
        return false;

    double value;
    switch (AspDataGetType(entry))
    {
        default:
            return false;

        case DataType_Boolean:
            value = AspDataGetBoolean(entry) ? 1 : 0;
            break;

        case DataType_Integer:
            value = AspDataGetInteger(entry);
            break;

        case DataType_Float:
            value = AspDataGetFloat(entry);
            break;
    }

    if (result != 0)
        *result = value;

    return true;
}

bool AspRangeValues
    (AspEngine *engine, const AspDataEntry *entry,
     int32_t *start, int32_t *end, int32_t *step)
{
    if (!AspIsRange(entry))
        return false;

    AspGetRange(engine, entry, start, end, step);
    return true;
}

/* If supplied, *size is the number of bytes in the string without null
   termination. If *size - index < bufferSize, this routine will deposit a
   null after the requested string content. If *size - index >= bufferSize,
   no null termination occurs. Negative indices are not supported. */
bool AspStringValue
    (AspEngine *engine, const AspDataEntry *const_entry,
     size_t *size, char *buffer, size_t index, size_t bufferSize)
{
    if (!AspIsString(const_entry))
        return false;
    AspDataEntry *entry = (AspDataEntry *)const_entry;

    size_t localSize = (size_t)AspDataGetSequenceCount(entry);
    if (size != 0)
        *size = localSize;

    if (buffer != 0 && bufferSize != 0)
    {
        if (localSize > bufferSize)
            localSize = bufferSize;

        uint32_t iterationCount = 0;
        for (AspSequenceResult fragmentResult =
             AspSequenceNext(engine, entry, 0);
             iterationCount < engine->cycleDetectionLimit &&
             localSize > 0 && fragmentResult.element != 0;
             iterationCount++,
             fragmentResult = AspSequenceNext
                (engine, entry, fragmentResult.element))
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
    (AspEngine *engine, const AspDataEntry *const_entry, bool repr)
{
    AspDataEntry *result = NewObject(engine, DataType_String);
    if (result == 0)
        return 0;

    /* Avoid recursion by using the engine's stack. */
    AspDataEntry *entry = (AspDataEntry *)const_entry;
    AspDataEntry *startStackTop = engine->stackTop;
    AspDataEntry *next = 0;
    bool flag = false;
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        char buffer[100];
        char type = (DataType)AspDataGetType(entry);
        switch (type)
        {
            default:
                strcpy(buffer, "?");

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
                snprintf(buffer, sizeof buffer, "%d", i);
                break;
            }

            case DataType_Float:
            {
                double f;
                AspFloatValue(entry, &f);
                int count = snprintf(buffer, sizeof buffer, "%g", f);
                if (!isnan(f) && !isinf(f) &&
                    strchr(buffer, '.') == 0 && strchr(buffer, 'e') == 0)
                    strcat(buffer, ".0");
                break;
            }

            case DataType_Range:
            {
                int count = 0;
                int32_t start, end, step;
                AspRangeValues(engine, entry, &start, &end, &step);
                if (start != 0)
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "%d", start);
                count += snprintf
                    (buffer + count, sizeof buffer - count, "..");
                bool unbounded =
                    step < 0 && end == INT32_MIN ||
                    step > 0 && end == INT32_MAX;
                if (!unbounded)
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "%d", end);
                if (step != 1)
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         ":%d", step);
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
                     AspSequenceNext(engine, entry, 0);
                     iterationCount < engine->cycleDetectionLimit &&
                     nextResult.element != 0;
                     iterationCount++,
                     nextResult = AspSequenceNext
                        (engine, entry, nextResult.element))
                {
                    AspDataEntry *fragment = nextResult.value;
                    uint8_t fragmentSize =
                        AspDataGetStringFragmentSize(fragment);
                    char *fragmentData =
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
                    (engine, entry, next);
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
                AspDataEntry *valueStackEntry = AspPushNoUse
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

                AspDataEntry *value = 0;
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
                AspDataEntry *valueStackEntry = AspPushNoUse(engine, value);
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

            case DataType_Iterator:
            {
                uint32_t iterableIndex =
                    AspDataGetIteratorIterableIndex(entry);
                AspDataEntry *iterable = AspValueEntry
                    (engine, iterableIndex);
                strcpy(buffer, "<iter:");
                if (iterable == 0)
                    strcat(buffer, "?");
                else
                    strcat(buffer, TypeString(AspDataGetType(iterable)));
                uint32_t memberIndex =
                    AspDataGetIteratorMemberIndex(entry);
                if (memberIndex == 0)
                    strcat(buffer, " @end");
                strcat(buffer, ">");
                break;
            }

            case DataType_Function:
            {
                int count = 0;
                count += snprintf
                    (buffer + count, sizeof buffer - count,
                     "<func:");
                if (AspDataGetFunctionIsApp(entry))
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "app:%d",
                         AspDataGetFunctionSymbol(entry));
                else
                    count += snprintf
                        (buffer + count, sizeof buffer - count,
                         "@%7.7X",
                         AspDataGetFunctionCodeAddress(entry));
                count += snprintf
                    (buffer + count, sizeof buffer - count,
                     ">");
                break;
            }

            case DataType_Module:
                snprintf
                    (buffer, sizeof buffer, "<mod:@%7.7X>",
                     AspDataGetModuleCodeAddress(entry));
                break;

            case DataType_AppIntegerObject:
            case DataType_AppPointerObject:
            {
                const AspDataEntry *infoEntry = AspAppObjectInfoEntry
                    (engine, entry);
                snprintf
                    (buffer, sizeof buffer, "<app-%s:%d:",
                     type == DataType_AppIntegerObject ? "int" : "ptr",
                     AspDataGetAppObjectType(infoEntry));
                if (infoEntry == 0)
                    strcat(buffer, "?");
                else
                {
                    size_t
                        len = strlen(buffer),
                        remainingLen = sizeof buffer - len;
                    char *bufferEnd = buffer + len;
                    if (type == DataType_AppIntegerObject)
                    {
                        snprintf
                            (bufferEnd, remainingLen, "%d>",
                             AspDataGetAppIntegerObjectValue(infoEntry));
                    }
                    else
                    {
                        snprintf
                            (bufferEnd, remainingLen, "%p>",
                             AspDataGetAppPointerObjectValue(infoEntry));
                    }
                }
                break;
            }

            case DataType_Type:
            {
                strcpy(buffer, "<type ");
                strcat(buffer, TypeString(AspDataGetTypeValue(entry)));
                strcat(buffer, ">");
                break;
            }
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
        case DataType_Iterator:
            return "iter";
        case DataType_Function:
            return "func";
        case DataType_Module:
            return "mod";
        case DataType_AppIntegerObject:
            return "app-int";
        case DataType_AppPointerObject:
            return "app-ptr";
        case DataType_Type:
            return "type";
    }

    return "?";
}

unsigned AspCount(const AspDataEntry *entry)
{
    if (entry == 0)
        return 0;

    switch (AspDataGetType(entry))
    {
        default:
            return 1;

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
            return AspDataGetSequenceCount(entry);

        case DataType_Set:
        case DataType_Dictionary:
            return AspDataGetTreeCount(entry);
    }
}

AspDataEntry *AspElement
    (AspEngine *engine, AspDataEntry *sequence, int index)
{
    uint8_t type = AspDataGetType(sequence);
    if (type != DataType_Tuple && type != DataType_List)
        return 0;

    AspSequenceResult result = AspSequenceIndex(engine, sequence, index);
    if (result.result != AspRunResult_OK)
        return 0;
    return result.value;
}

char AspStringElement
    (AspEngine *engine, const AspDataEntry *strEntry, int index)
{
    AspDataEntry *str = (AspDataEntry *)strEntry;

    if (AspDataGetType(str) != DataType_String)
        return 0;

    /* Treat negative indices as counting backwards from the end. */
    if (index < 0)
    {
        index = (int)AspDataGetSequenceCount(strEntry) + index;
        if (index < 0)
            return 0;
    }

    /* Locate the character within the applicable fragment. */
    AspSequenceResult nextResult = AspSequenceNext(engine, str, 0);
    uint32_t iterationCount = 0;
    for (;
         iterationCount < engine->cycleDetectionLimit &&
         nextResult.element != 0;
         iterationCount++,
         nextResult = AspSequenceNext(engine, str, nextResult.element))
    {
        AspDataEntry *fragment = nextResult.value;
        uint8_t fragmentSize = AspDataGetStringFragmentSize(fragment);
        if (index >= fragmentSize)
        {
            index -= fragmentSize;
            continue;
        }

        const uint8_t *stringData = AspDataGetStringFragmentData(fragment);
        return (char)stringData[index];
    }
    if (iterationCount >= engine->cycleDetectionLimit)
    {
        engine->runResult = AspRunResult_CycleDetected;
        return 0;
    }

    return 0;
}

AspDataEntry *AspFind
    (AspEngine *engine, AspDataEntry *tree, const AspDataEntry *key)
{
    AspTreeResult result = AspTreeFind(engine, tree, key);
    if (result.result != AspRunResult_OK)
        return 0;
    return AspIsSet(tree) ? result.key : result.value;
}

AspDataEntry *AspNext(AspEngine *engine, AspDataEntry *iterator)
{
    AspIteratorResult result = AspIteratorDereference(engine, iterator);
    if (result.result != AspRunResult_OK)
        return 0;
    AspIteratorNext(engine, iterator);
    return result.value;
}

bool AspAppObjectTypeValue
    (AspEngine *engine, const AspDataEntry *entry, int16_t *appType)
{
    if (!AspIsAppObject(entry))
        return false;

    if (appType != 0)
    {
        const AspDataEntry *infoEntry = AspAppObjectInfoEntry
            (engine, (AspDataEntry *)entry);
        *appType = AspDataGetAppObjectType(infoEntry);
    }

    return true;
}

bool AspAppIntegerObjectValues
    (AspEngine *engine, const AspDataEntry *entry,
     int16_t *appType, int32_t *value)
{
    if (!AspIsAppIntegerObject(entry))
        return false;

    if (appType != 0)
        AspAppObjectTypeValue(engine, entry, appType);
    if (value != 0)
    {
        const AspDataEntry *infoEntry = AspAppObjectInfoEntry
            (engine, (AspDataEntry *)entry);
        *value = AspDataGetAppIntegerObjectValue(infoEntry);
    }

    return true;
}

bool AspAppPointerObjectValues
    (AspEngine *engine, const AspDataEntry *entry,
     int16_t *appType, void **value)
{
    if (!AspIsAppPointerObject(entry))
        return false;

    if (appType != 0)
        AspAppObjectTypeValue(engine, entry, appType);
    if (value != 0)
    {
        const AspDataEntry *infoEntry = AspAppObjectInfoEntry
            (engine, (AspDataEntry *)entry);
        *value = AspDataGetAppPointerObjectValue(infoEntry);
    }

    return true;
}

AspDataEntry *AspNewNone(AspEngine *engine)
{
    return NewObject(engine, DataType_None);
}

AspDataEntry *AspNewEllipsis(AspEngine *engine)
{
    return NewObject(engine, DataType_Ellipsis);
}

AspDataEntry *AspNewBoolean(AspEngine *engine, bool value)
{
    /* Return one of the Boolean singletons. */
    AspDataEntry **singleton =
        value ? &engine->trueSingleton : &engine->falseSingleton;
    if (*singleton != 0)
        AspRef(engine, *singleton);
    else
    {
        /* Create the singleton. */
        *singleton = NewObject(engine, DataType_Boolean);
        if (*singleton != 0)
            AspDataSetBoolean(*singleton, value);
    }
    return *singleton;
}

AspDataEntry *AspNewInteger(AspEngine *engine, int32_t value)
{
    AspDataEntry *entry = NewObject(engine, DataType_Integer);
    if (entry != 0)
        AspDataSetInteger(entry, value);
    return entry;
}

AspDataEntry *AspNewFloat(AspEngine *engine, double value)
{
    AspDataEntry *entry = NewObject(engine, DataType_Float);
    if (entry != 0)
        AspDataSetFloat(entry, value);
    return entry;
}

AspDataEntry *AspNewRange
    (AspEngine *engine,
     int32_t startValue, int32_t endValue, int32_t stepValue)
{
    return NewRange(engine, startValue, &endValue, stepValue);
}

AspDataEntry *AspNewUnboundedRange
    (AspEngine *engine,
     int32_t startValue, int32_t stepValue)
{
    return NewRange(engine, startValue, 0, stepValue);
}

static AspDataEntry *NewRange
    (AspEngine *engine,
     int32_t startValue, int32_t *endValue, int32_t stepValue)
{
    AspDataEntry *entry = NewObject(engine, DataType_Range);
    if (entry != 0)
    {
        bool error = false;
        AspDataEntry *start = 0, *end = 0, *step = 0;
        if (!error && startValue != 0)
        {
            start = AspNewInteger(engine, startValue);
            if (start == 0)
                error = true;
        }
        if (!error && endValue != 0)
        {
            end = AspNewInteger(engine, *endValue);
            if (end == 0)
                error = true;
        }
        if (!error && stepValue != 1)
        {
            step = AspNewInteger(engine, stepValue);
            if (step == 0)
                error = true;
        }
        if (error)
        {
            AspUnref(engine, entry);
            entry = 0;
        }

        if (start != 0)
        {
            AspDataSetRangeHasStart(entry, true);
            AspDataSetRangeStartIndex(entry, AspIndex(engine, start));
        }
        if (end != 0)
        {
            AspDataSetRangeHasEnd(entry, true);
            AspDataSetRangeEndIndex(entry, AspIndex(engine, end));
        }
        if (step != 0)
        {
            AspDataSetRangeHasStep(entry, true);
            AspDataSetRangeStepIndex(entry, AspIndex(engine, step));
        }
    }

    return entry;
}

AspDataEntry *AspNewString
    (AspEngine *engine, const char *buffer, size_t bufferSize)
{
    AspDataEntry *entry = NewObject(engine, DataType_String);
    if (entry == 0)
        return 0;

    AspRunResult appendResult = AspStringAppendBuffer
        (engine, entry, buffer, bufferSize);
    if (appendResult != AspRunResult_OK)
    {
        AspFree(engine, AspIndex(engine, entry));
        entry = 0;
    }

    return entry;
}

AspDataEntry *AspNewTuple(AspEngine *engine)
{
    return NewObject(engine, DataType_Tuple);
}

AspDataEntry *AspNewList(AspEngine *engine)
{
    return NewObject(engine, DataType_List);
}

AspDataEntry *AspNewSet(AspEngine *engine)
{
    return NewObject(engine, DataType_Set);
}

AspDataEntry *AspNewDictionary(AspEngine *engine)
{
    return NewObject(engine, DataType_Dictionary);
}

AspDataEntry *AspNewIterator(AspEngine *engine, AspDataEntry *iterable)
{
    AspIteratorResult result = AspIteratorCreate(engine, iterable);
    return result.result != AspRunResult_OK ? 0 : result.value;
}

AspDataEntry *AspNewAppIntegerObject
    (AspEngine *engine, int16_t appType, int32_t value,
     void (*destructor)(AspEngine *, int16_t, int32_t))
{
    AspDataEntry *entry = NewObject(engine, DataType_AppIntegerObject);
    if (entry != 0)
    {
        AspDataEntry *infoEntry = entry;

        #ifdef ASP_WIDE_PTR
        infoEntry = NewObject(engine, DataType_AppIntegerObjectInfo);
        if (infoEntry == 0)
        {
            AspUnref(engine, entry);
            return 0;
        }
        AspDataSetAppObjectInfoIndex
            (entry, AspIndex(engine, infoEntry));
        #endif

        AspDataSetAppObjectType(infoEntry, appType);
        AspDataSetAppIntegerObjectValue(infoEntry, value);
        AspDataSetAppIntegerObjectDestructor(entry, destructor);
    }

    return entry;
}

AspDataEntry *AspNewAppPointerObject
    (AspEngine *engine, int16_t appType, void *value,
     void (*destructor)(AspEngine *, int16_t, void *))
{
    AspDataEntry *entry = NewObject(engine, DataType_AppPointerObject);
    if (entry != 0)
    {
        AspDataEntry *infoEntry = entry;

        #ifdef ASP_WIDE_PTR
        infoEntry = NewObject
            (engine, DataType_AppPointerObjectInfo);
        if (infoEntry == 0)
        {
            AspUnref(engine, entry);
            return 0;
        }
        AspDataSetAppObjectInfoIndex
            (entry, AspIndex(engine, infoEntry));
        #endif

        AspDataSetAppObjectType(infoEntry, appType);
        AspDataSetAppPointerObjectValue(infoEntry, value);
        AspDataSetAppPointerObjectDestructor(entry, destructor);
    }

    return entry;
}

AspDataEntry *AspNewType(AspEngine *engine, const AspDataEntry *object)
{
    AspDataEntry *entry = NewObject(engine, DataType_Type);
    if (entry != 0)
        AspDataSetTypeValue(entry, AspDataGetType(object));
    return entry;
}

static AspDataEntry *NewObject(AspEngine *engine, DataType type)
{
    AspDataEntry *entry = AspAllocEntry(engine, type);
    if (entry == 0)
    {
        engine->runResult = AspRunResult_OutOfDataMemory;
        return 0;
    }
    return entry;
}

bool AspTupleAppend
    (AspEngine *engine, AspDataEntry *tuple, AspDataEntry *value, bool take)
{
    /* Ensure the container is a tuple that is not referenced anywhere else. */
    AspAssert(engine, AspDataGetType(tuple) == DataType_Tuple);
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetUseCount(tuple) == 1);
    if (assertResult != AspRunResult_OK)
        return false;

    AspSequenceResult result = AspSequenceAppend(engine, tuple, value);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspListAppend
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *value, bool take)
{
    /* Ensure the container is a list, not a tuple. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(list) == DataType_List);
    if (assertResult != AspRunResult_OK)
        return false;

    AspSequenceResult result = AspSequenceAppend(engine, list, value);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspListInsert
    (AspEngine *engine, AspDataEntry *list,
     int index, AspDataEntry *value, bool take)
{
    /* Ensure the container is a list, not a tuple. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(list) == DataType_List);
    if (assertResult != AspRunResult_OK)
        return false;

    AspSequenceResult result = AspSequenceInsertByIndex
        (engine, list, index, value);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspListErase(AspEngine *engine, AspDataEntry *list, int index)
{
    /* Ensure the container is a list, not a tuple. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(list) == DataType_List);
    if (assertResult != AspRunResult_OK)
        return false;

    return AspSequenceErase(engine, list, index, true);
}

bool AspStringAppend
    (AspEngine *engine, AspDataEntry *str,
     const char *buffer, size_t bufferSize)
{
    /* Ensure we're using a string that is not referenced anywhere else. */
    AspAssert(engine, AspDataGetType(str) == DataType_String);
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetUseCount(str) == 1);
    if (assertResult != AspRunResult_OK)
        return false;

    AspRunResult result = AspStringAppendBuffer
        (engine, str, buffer, bufferSize);
    return result == AspRunResult_OK;
}

bool AspSetInsert
    (AspEngine *engine, AspDataEntry *set, AspDataEntry *key, bool take)
{
    /* Ensure the container is a set. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(set) == DataType_Set);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult result = AspTreeInsert
        (engine, set, key, 0);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, key);

    return true;
}

bool AspSetErase(AspEngine *engine, AspDataEntry *set, AspDataEntry *key)
{
    /* Ensure the container is a set. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(set) == DataType_Set);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult findResult = AspTreeFind(engine, set, key);
    if (findResult.result != AspRunResult_OK)
        return false;
    AspRunResult result = AspTreeEraseNode
        (engine, set, findResult.node, true, true);
    return result == AspRunResult_OK;
}

bool AspDictionaryInsert
    (AspEngine *engine, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value, bool take)
{
    /* Ensure the container is a dictionary. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(dictionary) == DataType_Dictionary);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult result = AspTreeInsert
        (engine, dictionary, key, value);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
    {
        AspUnref(engine, key);
        AspUnref(engine, value);
    }

    return true;
}

bool AspDictionaryErase
    (AspEngine *engine, AspDataEntry *dictionary, AspDataEntry *key)
{
    /* Ensure the container is a dictionary. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(dictionary) == DataType_Dictionary);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult findResult = AspTreeFind(engine, dictionary, key);
    if (findResult.result != AspRunResult_OK)
        return false;
    AspRunResult result = AspTreeEraseNode
        (engine, dictionary, findResult.node, true, true);
    return result == AspRunResult_OK;
}

AspDataEntry *AspArguments(AspEngine *engine)
{
    AspTreeResult findResult = AspFindSymbol
        (engine, engine->systemNamespace, AspSystemArgumentsSymbol);
    return findResult.value;
}

void *AspContext(const AspEngine *engine)
{
    return engine->context;
}

bool AspAgain(const AspEngine *engine)
{
    return engine->again;
}

AspRunResult AspAssert(AspEngine *engine, bool condition)
{
    /* Bail if a previous error condition exists. */
    if (engine->runResult != AspRunResult_OK)
        return engine->runResult;

    /* Check the given condition. */
    if (condition)
        return AspRunResult_OK;

    /* Indicate the condition failure. */
    return engine->runResult = AspRunResult_InternalError;
}
