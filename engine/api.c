/*
 * Asp API implementation.
 */

#include "asp.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

static AspDataEntry *AspNewObject(AspEngine *, DataType);

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
        type == DataType_Boolean ||
        type == DataType_Integer;
}

bool AspIsNumber(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        type == DataType_Integer ||
        type == DataType_Float;
}

bool AspIsNumeric(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        type == DataType_Boolean ||
        type == DataType_Integer ||
        type == DataType_Float;
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

bool AspIsSet(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Set;
}

bool AspIsDictionary(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Dictionary;
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
            if (f < INT_MIN)
            {
                value = INT_MIN;
                valid = false;
            }
            else if (f > INT_MAX)
            {
                value = INT_MAX;
                valid = false;
            }
            else
                value = (int)f;
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
    AspDataEntry *entry = (AspDataEntry *)const_entry;
    if (!AspIsString(entry))
        return false;

    size_t localSize = (size_t)AspDataGetSequenceCount(entry);
    if (size != 0)
        *size = localSize;

    if (buffer != 0 && bufferSize != 0)
    {
        if (localSize > bufferSize)
            localSize = bufferSize;

        for (AspSequenceResult fragmentResult =
             AspSequenceNext(engine, entry, 0);
             localSize > 0 && fragmentResult.element != 0;
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

    char buffer[100];
    if (AspIsNone(entry))
        strcpy(buffer, "None");
    else if (AspIsEllipsis(entry))
        strcpy(buffer, "...");
    else if (AspIsBoolean(entry))
        strcpy(buffer, AspIsTrue(engine, entry) ? "True" : "False");
    else if (AspIsInteger(entry))
    {
        int32_t i;
        AspIntegerValue(entry, &i);
        sprintf(buffer, "%d", i);
    }
    else if (AspIsFloat(entry))
    {
        double f;
        AspFloatValue(entry, &f);
        sprintf(buffer, "%g", f);
    }
    else if (AspIsRange(entry))
    {
        int count = 0;
        int32_t start, end, step;
        AspRangeValues(engine, entry, &start, &end, &step);
        if (start != 0)
            count += sprintf(buffer + count, "%d", start);
        count += sprintf(buffer + count, "..");
        bool unbounded =
            step < 0 && end == INT32_MIN ||
            step > 0 && end == INT32_MAX;
        if (!unbounded)
            count += sprintf(buffer + count, "%d", end);
        if (step != 1)
            count += sprintf(buffer + count, ":%d", step);
    }
    else if (AspIsType(entry))
    {
        const char *typeString = AspTypeString(entry);
        strcpy(buffer, typeString == 0 ? "?" : typeString);
    }
    else
    {
        /* TODO: Add support for remaining types. */
        strcpy(buffer, "<unsupported type>");
    }

    return AspNewString(engine, buffer, strlen(buffer));
}

const char *AspTypeString(const AspDataEntry *typeEntry)
{
    if (typeEntry == 0 || AspDataGetType(typeEntry) != DataType_Type)
        return 0;

    switch (AspDataGetTypeValue(typeEntry))
    {
        case DataType_None:
            return "<type None>";
        case DataType_Ellipsis:
            return "<type ...>";
        case DataType_Boolean:
            return "<type bool>";
        case DataType_Integer:
            return "<type int>";
        case DataType_Float:
            return "<type float>";
        case DataType_Range:
            return "<type range>";
        case DataType_String:
            return "<type str>";
        case DataType_Tuple:
            return "<type tuple>";
        case DataType_List:
            return "<type list>";
        case DataType_Set:
            return "<type set>";
        case DataType_Dictionary:
            return "<type dict>";
        case DataType_Iterator:
            return "<type iter>";
        case DataType_Function:
            return "<type func>";
        case DataType_Module:
            return "<type mod>";
        case DataType_Type:
            return "<type type>";
    }

    return "<type ?>";
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

AspDataEntry *AspListElement
    (AspEngine *engine, AspDataEntry *list, int index)
{
    uint8_t type = AspDataGetType(list);
    if (type != DataType_Tuple && type != DataType_List)
        return 0;

    AspSequenceResult result = AspSequenceIndex(engine, list, index);
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

    AspSequenceResult nextResult = AspSequenceNext(engine, str, 0);
    for (; nextResult.element != 0;
         nextResult = AspSequenceNext(engine, str, nextResult.element))
    {
        AspDataEntry *fragment = nextResult.value;
        uint8_t fragmentSize = AspDataGetStringFragmentSize(fragment);
        if (index >= fragmentSize)
            continue;

        const uint8_t *stringData = AspDataGetStringFragmentData(fragment);
        return (char)stringData[index];
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

AspDataEntry *AspNewNone(AspEngine *engine)
{
    return AspNewObject(engine, DataType_None);
}

AspDataEntry *AspNewEllipsis(AspEngine *engine)
{
    return AspNewObject(engine, DataType_Ellipsis);
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
        *singleton = AspNewObject(engine, DataType_Boolean);
        AspDataSetBoolean(*singleton, value);
    }
    return *singleton;
}

AspDataEntry *AspNewInteger(AspEngine *engine, int32_t value)
{
    AspDataEntry *entry = AspNewObject(engine, DataType_Integer);
    if (entry != 0)
        AspDataSetInteger(entry, value);
    return entry;
}

AspDataEntry *AspNewFloat(AspEngine *engine, double value)
{
    AspDataEntry *entry = AspNewObject(engine, DataType_Float);
    if (entry != 0)
        AspDataSetFloat(entry, value);
    return entry;
}

AspDataEntry *AspNewString
    (AspEngine *engine, const char *buffer, size_t bufferSize)
{
    AspDataEntry *entry = AspNewObject(engine, DataType_String);
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
    return AspNewObject(engine, DataType_Tuple);
}

AspDataEntry *AspNewList(AspEngine *engine)
{
    return AspNewObject(engine, DataType_List);
}

AspDataEntry *AspNewSet(AspEngine *engine)
{
    return AspNewObject(engine, DataType_Set);
}

AspDataEntry *AspNewDictionary(AspEngine *engine)
{
    return AspNewObject(engine, DataType_Dictionary);
}

AspDataEntry *AspNewType(AspEngine *engine, AspDataEntry *object)
{
    AspDataEntry *entry = AspNewObject(engine, DataType_Type);
    if (entry != 0)
        AspDataSetTypeValue(entry, AspDataGetType(object));
    return entry;
}

static AspDataEntry *AspNewObject(AspEngine *engine, DataType type)
{
    AspDataEntry *entry = AspAllocEntry(engine, type);
    if (entry == 0)
    {
        engine->runResult = AspRunResult_OutOfDataMemory;
        return 0;
    }
    return entry;
}

bool AspListAppend
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *value)
{
    /* Ensure the container is a list, not a tuple. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(list) == DataType_List);
    if (assertResult != AspRunResult_OK)
        return false;

    AspSequenceResult result = AspSequenceAppend(engine, list, value);
    return result.result == AspRunResult_OK;
}

bool AspListInsert
    (AspEngine *engine, AspDataEntry *list,
     int index, AspDataEntry *value)
{
    /* Ensure the container is a list, not a tuple. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(list) == DataType_List);
    if (assertResult != AspRunResult_OK)
        return false;

    AspSequenceResult result = AspSequenceInsertByIndex
        (engine, list, index, value);
    return result.result == AspRunResult_OK;
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
    (AspEngine *engine, AspDataEntry *set, AspDataEntry *key)
{
    /* Ensure the container is a set. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(set) == DataType_Set);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult result = AspTreeInsert(engine, set, key, 0);
    return result.result == AspRunResult_OK;
}

bool AspDictionaryInsert
    (AspEngine *engine, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value)
{
    /* Ensure the container is a dictionary. */
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(dictionary) == DataType_Dictionary);
    if (assertResult != AspRunResult_OK)
        return false;

    AspTreeResult result = AspTreeInsert(engine, dictionary, key, value);
    return result.result == AspRunResult_OK;
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
