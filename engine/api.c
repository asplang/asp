/*
 * Asp API implementation.
 */

#include "asp.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include <string.h>

static AspDataEntry *AspNewObject(AspEngine *, DataType);

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

bool AspIsNumeric(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        type == DataType_Boolean ||
        type == DataType_Integer ||
        type == DataType_Float;
}

bool AspIsNumber(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
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

        case DataType_String:
        case DataType_List:
            return AspDataGetSequenceCount(entry) != 0;

        case DataType_Tuple:
            /* TODO: Add testing of element if there's only 1. */
            return AspDataGetSequenceCount(entry) != 0;

        case DataType_Set:
        case DataType_Dictionary:
            return AspDataGetTreeCount(entry) != 0;

        case DataType_Iterator:
            /* Different than Python. */
            return AspDataGetIteratorMemberIndex(entry) != 0;

        case DataType_Range:
        {
            /* Different than Python. */
            int32_t startValue, endValue, stepValue;
            AspGetRange(engine, entry, &startValue, &endValue, &stepValue);
            return !AspIsValueAtRangeEnd(startValue, endValue, stepValue);
        }
    }
}

bool AspIntegerValue(const AspDataEntry *entry, int32_t *result)
{
    if (!AspIsNumeric(entry))
        return false;

    int value;
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
            /* TODO: Check against INT_MIN and INT_MAX. */
            value = (int)AspDataGetFloat(entry);
            break;
    }

    if (result != 0)
        *result = value;

    return true;
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

/* If supplied, *size is the number of bytes in the string without null
   termination. If *size - index < bufferSize, this routine will deposit a
   null after the requested string content. If *size - index >= bufferSize,
   no null termination occurs. */
bool AspStringValue
    (AspEngine *engine, const AspDataEntry *const_entry,
     size_t *size, char *buffer, size_t index, size_t bufferSize)
{
    AspDataEntry *entry = (AspDataEntry *)const_entry;
    if (entry == 0 || AspDataGetType(entry) != DataType_String)
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

unsigned AspCount(const AspDataEntry *entry)
{
    if (entry == 0)
        return 0;

    switch (AspDataGetType(entry))
    {
        default:
            return 1;

        case DataType_String:
        case DataType_List:
            return AspDataGetSequenceCount(entry);

        case DataType_Set:
        case DataType_Dictionary:
            return AspDataGetTreeCount(entry);
    }
}

AspDataEntry *AspListElement
    (AspEngine *engine, AspDataEntry *list, unsigned index)
{
    AspSequenceResult result = AspSequenceIndex(engine, list, index);
    if (result.result != AspRunResult_OK)
        return 0;
    return result.value;
}

char AspStringElement
    (AspEngine *engine, AspDataEntry *str, unsigned index)
{
    /* TODO: Implement. Share code with IDX instruction in engine.c.
       Look at string iterator code for possible logic to borrow. */
    engine->runResult = AspRunResult_NotImplemented;
    return 0;
}

AspDataEntry *AspFind
    (AspEngine *engine, AspDataEntry *tree, AspDataEntry *key)
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
    AspDataEntry *entry = AspNewObject(engine, DataType_Boolean);
    if (entry != 0)
        AspDataSetBoolean(entry, value);
    return entry;
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

AspDataEntry *AspNewString(AspEngine *engine, const char *value)
{
    /* TODO: Implement using logic from engine.c, PUSHS instruction. */
    engine->runResult = AspRunResult_NotImplemented;
    return 0;
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
    AspSequenceResult result = AspSequenceAppend(engine, list, value);
    return result.result == AspRunResult_OK;
}

bool AspStringAppend
    (AspEngine *engine, AspDataEntry *str, char c)
{
    /* TODO: Implement. */
    engine->runResult = AspRunResult_NotImplemented;
    return 0;
}

bool AspSetInsert
    (AspEngine *engine, AspDataEntry *set, AspDataEntry *key)
{
    AspTreeResult result = AspTreeInsert(engine, set, key, 0);
    return result.result == AspRunResult_OK;
}

bool AspDictionaryInsert
    (AspEngine *engine, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value)
{
    AspTreeResult result = AspTreeInsert(engine, dictionary, key, value);
    return result.result == AspRunResult_OK;
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
