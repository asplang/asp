/*
 * Asp API implementation.
 */

#include "asp.h"
#include "range.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"
#include "iterator.h"
#include "assign.h"
#include "function.h"
#include "reserved.h"
#include "compare.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#if !defined ASP_ENGINE_VERSION_MAJOR || \
    !defined ASP_ENGINE_VERSION_MINOR || \
    !defined ASP_ENGINE_VERSION_PATCH || \
    !defined ASP_ENGINE_VERSION_TWEAK
#error ASP_ENGINE_VERSION_* macros undefined
#endif

static AspDataEntry *NewRange
    (AspEngine *, int32_t start, const int32_t *end, int32_t step);
static AspDataEntry *NewObject(AspEngine *, DataType);
static AspDataEntry *GetNamespace(AspEngine *, const AspDataEntry *);
static bool PrepareArgumentList(AspEngine *);

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

bool AspIsSymbol(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Symbol;
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

bool AspIsSimpleObject(const AspDataEntry *entry)
{
    return
        entry != 0 && AspDataGetType(entry) == DataType_Object
        #ifdef ASP_FEATURE_CLASS
        && AspDataGetObjectClassIndex(entry) == 0
        #endif
        ;
}

#ifdef ASP_FEATURE_CLASS

bool AspIsClassInstance(const AspDataEntry *entry)
{
    return
        entry != 0 && AspDataGetType(entry) == DataType_Object &&
        AspDataGetObjectClassIndex(entry) != 0;
}

bool AspIsClass(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Class;
}

bool AspIsBoundMethod(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_BoundMethod;
}

bool AspIsSuper(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Super;
}

#endif

bool AspIsFunction(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Function;
}

bool AspIsModule(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_Module;
}

bool AspIsReverseIterator(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_ReverseIterator;
}

bool AspIsForwardIterator(const AspDataEntry *entry)
{
    return entry != 0 && AspDataGetType(entry) == DataType_ForwardIterator;
}

bool AspIsIterator(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_ReverseIterator ||
         type == DataType_ForwardIterator);
}

bool AspIsIterable(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        entry != 0 &&
        (type == DataType_Ellipsis ||
         type == DataType_Range ||
         type == DataType_String ||
         type == DataType_Tuple ||
         type == DataType_List ||
         type == DataType_Set ||
         type == DataType_Dictionary ||
         type == DataType_Object ||
         #ifdef ASP_FEATURE_CLASS
         type == DataType_Class ||
         #endif
         type == DataType_Module);
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
            bool bounded;
            AspGetRange
                (engine, entry, &startValue, &endValue, &stepValue, &bounded);
            return !AspIsValueAtRangeEnd
                (startValue, endValue, stepValue, bounded);
        }

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
            return AspDataGetSequenceCount(entry) != 0;

        case DataType_Set:
        case DataType_Dictionary:
            return AspDataGetTreeCount(entry) != 0;

        case DataType_ForwardIterator:
        case DataType_ReverseIterator:
            return AspDataGetIteratorMemberIndex(entry) != 0;

        case DataType_Type:
            return AspDataGetTypeValue(entry) != DataType_None;
    }
}

bool AspIsTypeOf
    (AspEngine *engine, const AspDataEntry *object, const AspDataEntry *type)
{
    #ifdef ASP_FEATURE_CLASS
    if (AspIsClassInstance(object) && AspIsClass(type))
    {
        uint32_t classIndex = AspIndex(engine, type);
        uint32_t iterationCount = 0;
        for (uint32_t objectClassIndex = AspDataGetObjectClassIndex(object);
             iterationCount < engine->cycleDetectionLimit;
             iterationCount++,
             objectClassIndex = AspDataGetClassBaseClassIndex
                (AspValueEntry(engine, objectClassIndex)))
        {
            if (objectClassIndex == 0)
                break;
            if (objectClassIndex == classIndex)
                return true;
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;

        return false;
    }
    else
    {
    #endif
        return
            AspIsType(type) &&
            AspDataGetType(object) == AspDataGetTypeValue(type);
    #ifdef ASP_FEATURE_CLASS
    }
    #endif
}

#ifdef ASP_FEATURE_CLASS

bool AspIsSubclassOf
    (AspEngine *engine, const AspDataEntry *class1, const AspDataEntry *class2)
{
    if (!AspIsClass(class1) || !AspIsClass(class2))
        return false;

    uint32_t class2Index = AspIndex(engine, class2);
    uint32_t iterationCount = 0;
    for (uint32_t class1Index = AspIndex(engine, class1);
         iterationCount < engine->cycleDetectionLimit;
         iterationCount++,
         class1Index = AspDataGetClassBaseClassIndex
            (AspValueEntry(engine, class1Index)))
    {
        if (class1Index == 0)
            break;
        if (class1Index == class2Index)
            return true;
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    return false;
}

#endif

bool AspIntegerValue(const AspDataEntry *entry, int32_t *result)
{
    if (!AspIsNumeric(entry))
        return false;

    int32_t value;
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
                value = (int32_t)round(f);
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

bool AspSymbolValue(const AspDataEntry *entry, int32_t *result)
{
    if (!AspIsSymbol(entry))
        return false;

    int32_t value = AspDataGetSymbol(entry);

    if (result != 0)
        *result = value;

    return true;
}

bool AspRangeValues
    (AspEngine *engine, const AspDataEntry *entry,
     int32_t *start, int32_t *end, int32_t *step, bool *bounded)
{
    if (!AspIsRange(entry))
        return false;

    AspGetRange(engine, entry, start, end, step, bounded);
    return true;
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

AspRunResult AspCount
    (AspEngine *engine, const AspDataEntry *entry, int32_t *count)
{
    if (entry == 0)
        return 0;

    switch (AspDataGetType(entry))
    {
        default:
            *count = 1;
            break;

        case DataType_Range:
            return AspRangeCount(engine, entry, count);

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
            *count = AspDataGetSequenceCount(entry);
            break;

        case DataType_Set:
        case DataType_Dictionary:
            *count = AspDataGetTreeCount(entry);
            break;
    }

    return AspRunResult_OK;
}

AspDataEntry *AspElement
    (AspEngine *engine, const AspDataEntry *sequence, int32_t index)
{
    uint8_t type = AspDataGetType(sequence);
    if (type != DataType_Tuple && type != DataType_List)
        return 0;

    AspSequenceResult result = AspSequenceIndex(engine, sequence, index);
    if (result.result != AspRunResult_OK)
        return 0;
    return result.value;
}

int32_t AspRangeElement
    (AspEngine *engine, const AspDataEntry *range, int32_t index)
{
    if (AspDataGetType(range) != DataType_Range)
        return 0;

    AspRangeResult result = AspRangeIndex(engine, range, index, false);
    if (result.result != AspRunResult_OK)
        return 0;
    return result.intValue;
}

char AspStringElement
    (AspEngine *engine, const AspDataEntry *str, int32_t index)
{
    if (AspDataGetType(str) != DataType_String)
        return '\0';

    char c;
    AspRunResult result = AspStringIndex(engine, str, index, &c);
    if (result != AspRunResult_OK)
        return '\0';
    return c;
}

AspDataEntry *AspFind
    (AspEngine *engine, const AspDataEntry *tree, const AspDataEntry *key)
{
    AspTreeResult result = AspTreeFind(engine, tree, key);
    if (result.result != AspRunResult_OK)
        return 0;
    return AspIsSet(tree) ? result.key : result.value;
}

AspDataEntry *AspMember
    (AspEngine *engine, const AspDataEntry *object, int32_t symbol)
{
    AspDataEntry *ns = GetNamespace(engine, object);
    if (ns == 0)
        return 0;

    AspTreeResult result = AspFindSymbol(engine, ns, symbol);
    if (result.result != AspRunResult_OK)
        return 0;
    return result.value;
}

AspDataEntry *AspAt(AspEngine *engine, const AspDataEntry *iterator)
{
    AspIteratorResult result = AspIteratorDereference(engine, iterator);
    return result.result != AspRunResult_OK ? 0 : result.value;
}

bool AspAtSame
    (AspEngine *engine,
     const AspDataEntry *iterator1, const AspDataEntry *iterator2)
{
    if (!AspIsIterator(iterator1) || !AspIsIterator(iterator2))
        return false;
    int compareResult;
    AspRunResult result = AspCompare
        (engine, iterator1, iterator2, AspCompareType_Equality,
         &compareResult, 0);
    return result == AspRunResult_OK && compareResult == 0;
}

AspDataEntry *AspNext(AspEngine *engine, AspDataEntry *iterator)
{
    AspDataEntry *result = AspAt(engine, iterator);
    if (result == 0)
        return 0;
    AspIteratorNext(engine, iterator);
    return result;
}

AspDataEntry *AspIterable(AspEngine *engine, const AspDataEntry *iterator)
{
    if (!AspIsIterator(iterator))
        return 0;
    return AspValueEntry
        (engine, AspDataGetIteratorIterableIndex(iterator));
}

#ifdef ASP_FEATURE_CLASS

ASP_API AspDataEntry *AspInstanceClass
    (AspEngine *engine, const AspDataEntry *instance)
{
    if (!AspIsClassInstance(instance))
        return 0;
    return AspValueEntry
        (engine, AspDataGetObjectClassIndex(instance));
}

#endif

AspDataEntry *AspNewNone(AspEngine *engine)
{
    return NewObject(engine, DataType_None);
}

AspDataEntry *AspNewEllipsis(AspEngine *engine)
{
    /* Return the Ellipsis singleton. */
    AspDataEntry **singleton = &engine->ellipsisSingleton;
    if (*singleton != 0)
        AspRef(engine, *singleton);
    else
    {
        /* Create the singleton. */
        *singleton = NewObject(engine, DataType_Ellipsis);
    }
    return *singleton;
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

AspDataEntry *AspNewSymbol(AspEngine *engine, int32_t value)
{
    AspDataEntry *entry = NewObject(engine, DataType_Symbol);
    if (entry != 0)
        AspDataSetSymbol(entry, value);
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
     int32_t startValue, const int32_t *endValue, int32_t stepValue)
{
    AspDataEntry *entry = NewObject(engine, DataType_Range);
    if (entry != 0)
    {
        bool error = false;
        AspDataEntry *start = 0, *end = 0, *step = 0;
        if (!error && startValue != (stepValue < 0 ? -1 : 0))
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
            if (start)
                AspUnref(engine, start);
            if (end)
                AspUnref(engine, end);
            if (step)
                AspUnref(engine, step);
            AspUnref(engine, entry);
            return 0;
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

AspDataEntry *AspNewSimpleObject(AspEngine *engine)
{
    AspDataEntry *ns = AspAllocEntry(engine, DataType_Namespace);
    if (ns == 0)
        return 0;

    AspDataEntry *object = AspAllocEntry(engine, DataType_Object);
    if (object != 0)
        AspDataSetObjectNamespaceIndex(object, AspIndex(engine, ns));

    return object;
}

AspDataEntry *AspNewIterator
    (AspEngine *engine, AspDataEntry *iterable, bool reversed)
{
    AspIteratorResult result = AspIteratorCreate
        (engine, iterable, reversed);
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
    if (object == 0)
        return 0;

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
    if (tuple == 0 || AspDataGetType(tuple) != DataType_Tuple ||
        AspDataGetUseCount(tuple) != 1)
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
    if (list == 0 || AspDataGetType(list) != DataType_List)
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
     int32_t index, AspDataEntry *value, bool take)
{
    /* Ensure the container is a list, not a tuple. */
    if (list == 0 || AspDataGetType(list) != DataType_List)
        return false;

    AspSequenceResult result = AspSequenceInsertByIndex
        (engine, list, index, value);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspListErase(AspEngine *engine, AspDataEntry *list, int32_t index)
{
    /* Ensure the container is a list, not a tuple. */
    if (list == 0 || AspDataGetType(list) != DataType_List)
        return false;

    return AspSequenceErase(engine, list, index, true);
}

bool AspInsertAt
    (AspEngine *engine, AspDataEntry *iterator, AspDataEntry *value, bool take)
{
    AspRunResult result = AspIteratorInsert(engine, iterator, value, take);
    return result == AspRunResult_OK;
}

bool AspEraseAt
    (AspEngine *engine, AspDataEntry *iterator)
{
    AspRunResult result = AspIteratorErase(engine, iterator);
    return result == AspRunResult_OK;
}

bool AspStringAppend
    (AspEngine *engine, AspDataEntry *str,
     const char *buffer, size_t bufferSize)
{
    /* Ensure we're using a string that is not referenced anywhere else. */
    if (str == 0 || AspDataGetType(str) != DataType_String ||
        AspDataGetUseCount(str) != 1)
        return false;

    AspRunResult result = AspStringAppendBuffer
        (engine, str, buffer, bufferSize);
    return result == AspRunResult_OK;
}

bool AspSetInsert
    (AspEngine *engine, AspDataEntry *set, AspDataEntry *key, bool take)
{
    /* Ensure the container is a set. */
    if (set == 0 || AspDataGetType(set) != DataType_Set)
        return false;

    AspTreeResult result = AspTreeInsert
        (engine, set, key, 0);
    if (result.result != AspRunResult_OK)
        return false;

    if (take)
        AspUnref(engine, key);

    return true;
}

bool AspSetErase(AspEngine *engine, AspDataEntry *set, const AspDataEntry *key)
{
    /* Ensure the container is a set. */
    if (set == 0 || AspDataGetType(set) != DataType_Set)
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
    if (dictionary == 0 || AspDataGetType(dictionary) != DataType_Dictionary)
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
    (AspEngine *engine, AspDataEntry *dictionary, const AspDataEntry *key)
{
    /* Ensure the container is a dictionary. */
    if (dictionary == 0 || AspDataGetType(dictionary) != DataType_Dictionary)
        return false;

    AspTreeResult findResult = AspTreeFind(engine, dictionary, key);
    if (findResult.result != AspRunResult_OK)
        return false;
    AspRunResult result = AspTreeEraseNode
        (engine, dictionary, findResult.node, true, true);
    return result == AspRunResult_OK;
}

bool AspObjectInsert
    (AspEngine *engine, AspDataEntry *object,
     int32_t symbol, AspDataEntry *value, bool take)
{
    /* Access the underlying namespace. */
    AspDataEntry *ns = GetNamespace(engine, object);
    if (ns == 0)
        return false;

    AspTreeResult result = AspTreeTryInsertBySymbol
        (engine, ns, symbol, value);
    if (result.result != AspRunResult_OK)
        return false;
    if (!result.inserted)
    {
        AspRunResult assignResult = AspAssignSimple
            (engine, result.node, value);
        if (assignResult != AspRunResult_OK)
            return false;
    }

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspObjectErase(AspEngine *engine, AspDataEntry *object, int32_t symbol)
{
    /* Access the underlying namespace. */
    AspDataEntry *ns = GetNamespace(engine, object);
    if (ns == 0)
        return false;

    AspTreeResult findResult = AspFindSymbol(engine, ns, symbol);
    if (findResult.result != AspRunResult_OK)
        return false;
    AspRunResult result = AspTreeEraseNode
        (engine, ns, findResult.node, true, true);
    return result == AspRunResult_OK;
}

static AspDataEntry *GetNamespace
    (AspEngine *engine, const AspDataEntry *object)
{
    uint8_t type = AspDataGetType(object);
    AspDataEntry *ns = 0;
    switch (type)
    {
        default:
            return 0;

        case DataType_Object:
            ns = AspEntry(engine, AspDataGetObjectNamespaceIndex(object));
            break;

        case DataType_Module:
            ns = AspEntry(engine, AspDataGetModuleNamespaceIndex(object));
            break;
    }

    return ns;
}

AspDataEntry *AspArguments(AspEngine *engine)
{
    AspTreeResult findResult = AspFindSymbol
        (engine, engine->systemNamespace, AspReservedSymbol_SystemArguments);
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

bool AspAddPositionalArgument
    (AspEngine *engine, AspDataEntry *value, bool take)
{
    if (value == 0)
        return false;

    if (!PrepareArgumentList(engine))
        return false;

    AspDataEntry *argument = NewObject(engine, DataType_Argument);
    if (argument == 0)
        return false;
    if (!take)
        AspRef(engine, value);
    AspDataSetArgumentValueIndex(argument, AspIndex(engine, value));

    AspSequenceResult result = AspSequenceAppend
        (engine, engine->argumentList, argument);
    return result.result == AspRunResult_OK;
}

bool AspAddNamedArgument
    (AspEngine *engine, int32_t symbol, AspDataEntry *value, bool take)
{
    if (value == 0)
        return false;

    if (!PrepareArgumentList(engine))
        return false;

    AspDataEntry *argument = NewObject(engine, DataType_Argument);
    if (argument == 0)
        return false;
    if (!take)
        AspRef(engine, value);
    AspDataSetArgumentHasName(argument, true);
    AspDataSetArgumentSymbol(argument, symbol);
    AspDataSetArgumentValueIndex(argument, AspIndex(engine, value));

    AspSequenceResult result = AspSequenceAppend
        (engine, engine->argumentList, argument);
    return result.result == AspRunResult_OK;
}

bool AspAddIterableGroupArgument
    (AspEngine *engine, AspDataEntry *value, bool take)
{
    if (!PrepareArgumentList(engine))
        return false;

    AspRunResult result = AspExpandIterableGroupArgument
        (engine, engine->argumentList, value);
    if (result != AspRunResult_OK)
        return false;
    if (take)
        AspUnref(engine, value);
    return true;
}

bool AspAddDictionaryGroupArgument
    (AspEngine *engine, AspDataEntry *value, bool take)
{
    if (!PrepareArgumentList(engine))
        return false;

    AspRunResult result = AspExpandDictionaryGroupArgument
        (engine, engine->argumentList, value);
    if (result != AspRunResult_OK)
        return false;
    if (take)
        AspUnref(engine, value);
    return true;
}

static bool PrepareArgumentList(AspEngine *engine)
{
    if (engine->argumentList == 0)
        engine->argumentList = NewObject(engine, DataType_ArgumentList);
    return engine->argumentList != 0;
}

void AspClearFunctionArguments(AspEngine *engine)
{
    if (engine->argumentList != 0)
    {
        AspUnref(engine, engine->argumentList);
        engine->argumentList = 0;
    }
}

AspRunResult AspCall
    (AspEngine *engine, AspDataEntry *function)
{
    /* Ensure an argument list has been prepared. */
    if (!PrepareArgumentList(engine))
        return AspRunResult_OutOfDataMemory;

    /* Consume the argument list and call the function. */
    AspDataEntry *argumentList = engine->argumentList;
    engine->argumentList = 0;
    return AspCallFunction
        (engine, function, argumentList, true
         #ifdef ASP_FEATURE_CLASS
         , 0, 0, false
         #endif
        );
}

AspRunResult AspReturnValue(AspEngine *engine, AspDataEntry **returnValue)
{
    /* Ensure that we've returned from a function call (i.e., a return value
       has been generated. */
    if (!engine->callReturning)
    {
        #ifdef ASP_DEBUG
        printf("No return value present\n");
        #endif
        return AspRunResult_InvalidAppFunction;
    }

    AspDataEntry *value = AspTopValue(engine);
    if (value == 0)
        return AspRunResult_StackUnderflow;
    AspPopNoErase(engine);
    engine->callReturning = false;

    if (returnValue != 0)
        *returnValue = value;

    return AspRunResult_OK;
}

int32_t AspNextSymbol(AspEngine *engine)
{
    return engine->nextSymbol--;
}

AspDataEntry *AspLoadLocal(AspEngine *engine, int32_t symbol)
{
    AspTreeResult findResult = AspFindSymbol
        (engine, engine->appFunctionNamespace, symbol);
    return findResult.value;
}

bool AspStoreLocal
    (AspEngine *engine, int32_t symbol, AspDataEntry *value, bool take)
{
    AspTreeResult insertResult = AspTreeTryInsertBySymbol
        (engine, engine->appFunctionNamespace, symbol, value);
    if (insertResult.result != AspRunResult_OK)
        return false;
    if (!insertResult.inserted)
    {
        AspRunResult assignResult = AspAssignSimple
            (engine, insertResult.node, value);
        if (assignResult != AspRunResult_OK)
            return false;
    }

    if (take)
        AspUnref(engine, value);

    return true;
}

bool AspEraseLocal(AspEngine *engine, int32_t symbol)
{
    AspTreeResult findResult = AspFindSymbol
        (engine, engine->appFunctionNamespace, symbol);
    if (findResult.result != AspRunResult_OK)
        return false;
    AspRunResult result = AspTreeEraseNode
        (engine, engine->appFunctionNamespace, findResult.node, false, true);
    return result == AspRunResult_OK;
}

bool AspIsFeature(const AspEngine *engine, AspFeatureBits featureBits)
{
    return (engine->featureBits & featureBits) != 0;
}
