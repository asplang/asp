/*
 * Asp engine iterator implementation.
 */

#include "iterator.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include "integer.h"
#include "integer-result.h"

AspIteratorResult AspIteratorCreate
    (AspEngine *engine, AspDataEntry *iterable)
{
    AspIteratorResult result = {AspRunResult_OK, 0};

    result.result = AspAssert(engine, iterable != 0);
    if (result.result != AspRunResult_OK)
        return result;

    /* Create an iterator entry. */
    AspDataEntry *iterator = AspAllocEntry
        (engine, DataType_Iterator);
    if (iterator == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspRef(engine, iterable);
    AspDataSetIteratorIterableIndex
        (iterator, AspIndex(engine, iterable));

    /* Set the iterator specifics based on the iterable. */
    AspDataEntry *member = 0;
    switch (AspDataGetType(iterable))
    {
        default:
            result.result = AspRunResult_UnexpectedType;
            break;

        case DataType_Range:
        {
            /* Determine a start value. */
            int32_t startValue, endValue, stepValue;
            AspGetRange
                (engine, iterable,
                 &startValue, &endValue, &stepValue);
            bool atEnd = AspIsValueAtRangeEnd
                (startValue, endValue, stepValue);

            /* Create an integer set to the start value. */
            if (!atEnd)
            {
                AspDataEntry *value = AspAllocEntry
                    (engine, DataType_Integer);
                if (value == 0)
                {
                    result.result = AspRunResult_OutOfDataMemory;
                    break;
                }
                AspDataSetInteger(value, startValue);
                AspDataSetIteratorMemberNeedsCleanup(iterator, true);
                member = value;
            }

            break;
        }

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
        {
            AspSequenceResult startResult = AspSequenceNext
                (engine, iterable, 0);
            if (startResult.result != AspRunResult_OK)
            {
                result.result = startResult.result;
                break;
            }
            member = startResult.element;

            break;
        }

        case DataType_Set:
        case DataType_Dictionary:
        {
            AspTreeResult startResult = AspTreeNext
                (engine, iterable, 0, true);
            if (startResult.result != AspRunResult_OK)
            {
                result.result = startResult.result;
                break;
            }
            member = startResult.node;

            break;
        }
    }
    AspDataSetIteratorMemberIndex(iterator, AspIndex(engine, member));

    if (result.result != AspRunResult_OK)
    {
        AspUnref(engine, iterator);
        return result;
    }

    result.value = iterator;
    return result;
}

AspRunResult AspIteratorNext
    (AspEngine *engine, AspDataEntry *iterator)
{
    AspRunResult assertResult = AspAssert(engine, iterator != 0);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    if (AspDataGetType(iterator) != DataType_Iterator)
        return AspRunResult_UnexpectedType;

    /* Gain access to the underlying iterable. */
    AspDataEntry *iterable = AspValueEntry
        (engine, AspDataGetIteratorIterableIndex(iterator));

    /* Check if the iterator is already at its end. */
    AspDataEntry *member = AspEntry
        (engine, AspDataGetIteratorMemberIndex(iterator));
    if (member == 0)
        return AspRunResult_IteratorAtEnd;

    /* Advance the iterator. */
    switch (AspDataGetType(iterable))
    {
        default:
            return AspRunResult_UnexpectedType;

        case DataType_Range:
        {
            int32_t endValue, stepValue;

            if (AspDataGetType(member) != DataType_Integer)
                return AspRunResult_UnexpectedType;

            AspGetRange(engine, iterable, 0, &endValue, &stepValue);
            int32_t newValue;
            AspRunResult addResult = AspTranslateIntegerResult
                (AspAddIntegers
                    (AspDataGetInteger(member), stepValue, &newValue));
            if (addResult != AspRunResult_OK)
                return addResult;
            AspUnref(engine, member);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
            bool atEnd = AspIsValueAtRangeEnd
                (newValue, endValue, stepValue);
            if (atEnd)
            {
                AspDataSetIteratorMemberNeedsCleanup(iterator, false);
                member = 0;
            }
            else
            {
                AspDataEntry *value = AspAllocEntry
                    (engine, DataType_Integer);
                if (value == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetInteger(value, newValue);
                member = value;
            }

            break;
        }

        case DataType_String:
        {
            if (AspDataGetType(member) != DataType_Element)
                return AspRunResult_UnexpectedType;
            AspDataEntry *fragment = AspEntry
                (engine, AspDataGetElementValueIndex(member));
            if (AspDataGetType(fragment) != DataType_StringFragment)
                return AspRunResult_UnexpectedType;
            uint8_t fragmentSize =
                AspDataGetStringFragmentSize(fragment);

            uint8_t stringIndex =
                AspDataGetIteratorStringIndex(iterator);
            if (stringIndex + 1 < fragmentSize)
            {
                AspDataSetIteratorStringIndex
                    (iterator, stringIndex + 1);
                break;
            }

            AspDataSetIteratorStringIndex(iterator, 0);

            /* Fall through... */
        }

        case DataType_Tuple:
        case DataType_List:
        {
            if (AspDataGetType(member) != DataType_Element)
                return AspRunResult_UnexpectedType;
            AspSequenceResult nextResult = AspSequenceNext
                (engine, iterable, member);
            if (nextResult.result != AspRunResult_OK)
                return nextResult.result;
            member = nextResult.element;

            break;
        }

        case DataType_Set:
        case DataType_Dictionary:
        {
            uint8_t memberType = AspDataGetType(member);
            if (memberType != DataType_SetNode &&
                memberType != DataType_DictionaryNode)
                return AspRunResult_UnexpectedType;
            AspTreeResult nextResult = AspTreeNext
                (engine, iterable, member, true);
            if (nextResult.result != AspRunResult_OK)
                return nextResult.result;
            member = nextResult.node;

            break;
        }
    }

    /* Update the iterator. */
    AspDataSetIteratorMemberIndex(iterator, AspIndex(engine, member));
    return AspRunResult_OK;
}

AspIteratorResult AspIteratorDereference
    (AspEngine *engine, const AspDataEntry *iterator)
{
    AspIteratorResult result = {AspRunResult_OK, 0};

    result.result = AspAssert(engine, iterator != 0);
    if (result.result != AspRunResult_OK)
        return result;

    if (AspDataGetType(iterator) != DataType_Iterator)
    {
        result.result = AspRunResult_UnexpectedType;
        return result;
    }

    /* Gain access to the underlying iterable and current member. */
    const AspDataEntry *iterable = AspValueEntry
        (engine, AspDataGetIteratorIterableIndex(iterator));
    AspDataEntry *member = AspEntry
        (engine, AspDataGetIteratorMemberIndex(iterator));
    if (member == 0)
    {
        result.result = AspRunResult_IteratorAtEnd;
        return result;
    }

    /* Dereference the iterator, creating a new value object. */
    AspDataEntry *value = 0;
    switch (AspDataGetType(iterable))
    {
        default:
            result.result = AspRunResult_UnexpectedType;
            break;

        case DataType_Range:
        {
            if (AspDataGetType(member) != DataType_Integer)
            {
                result.result = AspRunResult_UnexpectedType;
                break;
            }
            value = member;
            AspRef(engine, value);
            break;
        }

        case DataType_String:
        {
            AspDataEntry *fragment = AspValueEntry
                (engine, AspDataGetElementValueIndex(member));
            if (AspDataGetType(fragment) != DataType_StringFragment)
            {
                result.result = AspRunResult_UnexpectedType;
                break;
            }
            uint8_t stringIndex =
                AspDataGetIteratorStringIndex(iterator);
            const uint8_t *stringData =
                AspDataGetStringFragmentData(fragment);
            uint8_t c = stringData[stringIndex];

            AspDataEntry *resultString = AspAllocEntry
                (engine, DataType_String);
            if (resultString == 0)
            {
                result.result = AspRunResult_OutOfDataMemory;
                break;
            }

            AspDataEntry *newFragment =
                AspAllocEntry(engine, DataType_StringFragment);
            if (newFragment == 0)
            {
                AspUnref(engine, resultString);
                result.result = AspRunResult_OutOfDataMemory;
                break;
            }
            AspDataSetStringFragment(newFragment, &c , 1);

            AspSequenceResult appendResult = AspSequenceAppend
                (engine, resultString, newFragment);
            if (appendResult.result != AspRunResult_OK)
            {
                AspUnref(engine, newFragment);
                AspUnref(engine, resultString);
                result.result = appendResult.result;
                break;
            }

            value = resultString;

            break;
        }

        case DataType_Tuple:
        case DataType_List:
            value = AspValueEntry
                (engine, AspDataGetElementValueIndex(member));
            AspRef(engine, value);
            break;

        case DataType_Set:
            value = AspValueEntry
                (engine, AspDataGetTreeNodeKeyIndex(member));
            AspRef(engine, value);
            break;

        case DataType_Dictionary:
        {
            AspDataEntry *key = AspValueEntry
                (engine, AspDataGetTreeNodeKeyIndex(member));
            AspDataEntry *entryValue = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(member));

            AspDataEntry *tuple =
                AspAllocEntry(engine, DataType_Tuple);
            if (tuple == 0)
            {
                result.result = AspRunResult_OutOfDataMemory;
                break;
            }

            AspSequenceResult keyAppendResult = AspSequenceAppend
                (engine, tuple, key);
            AspSequenceResult valueAppendResult = AspSequenceAppend
                (engine, tuple, entryValue);
            if (keyAppendResult.result != AspRunResult_OK)
                result.result = keyAppendResult.result;
            else if (valueAppendResult.result != AspRunResult_OK)
                result.result = valueAppendResult.result;
            if (result.result != AspRunResult_OK)
            {
                AspUnref(engine, tuple);
                break;
            }

            value = tuple;

            break;
        }
    }

    if (result.result != AspRunResult_OK)
        return result;

    result.value = value;
    return result;
}
