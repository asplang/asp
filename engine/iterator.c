/*
 * Asp engine iterator implementation.
 */

#include "iterator.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include "integer.h"
#include "integer-result.h"

static bool ReversedRangeIteratorAtEnd
    (int32_t testValue,
     int32_t startValue, int32_t endValue, int32_t stepValue);

AspIteratorResult AspIteratorCreate
    (AspEngine *engine, AspDataEntry *iterable, bool reversed)
{
    AspIteratorResult result = {AspRunResult_OK, 0};

    result.result = AspAssert(engine, iterable != 0);
    if (result.result != AspRunResult_OK)
        return result;

    /* Create an iterator entry. Note that the type may be changed later if
       it turns out the new iterator is reversed. */
    AspDataEntry *iterator = AspAllocEntry(engine, DataType_ForwardIterator);
    if (iterator == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }

    /* Check if the argument is already an iterator. */
    uint8_t iterableType = AspDataGetType(iterable);
    const AspDataEntry *oldIterator = 0;
    if (AspIsIterator(iterable))
    {
        /* Access the underlying iterable. */
        oldIterator = iterable;
        iterable = AspValueEntry
            (engine, AspDataGetIteratorIterableIndex(iterable));
    }

    /* Point the iterator to its iterable. */
    AspRef(engine, iterable);
    AspDataSetIteratorIterableIndex(iterator, AspIndex(engine, iterable));

    /* Set the iterator specifics based on the type of iterable. */
    AspDataEntry *member = 0;
    switch (iterableType)
    {
        default:
            result.result = AspRunResult_UnexpectedType;
            break;

        case DataType_Range:
        {
            /* Determine the start value. */
            int32_t startValue, endValue, stepValue;
            bool bounded;
            AspGetRange
                (engine, iterable,
                 &startValue, &endValue, &stepValue, &bounded);
            int32_t initialValue;
            bool atEnd;
            if (reversed)
            {
                int32_t count;
                result.result = AspRangeCount(engine, iterable, &count);
                if (result.result != AspRunResult_OK)
                    return result;
                atEnd = startValue == endValue;
                if (!atEnd && count > 0)
                {
                    /* Compute the start value as (start + (count - 1) * step).
                       Note that because count is always non-negative,
                       (count - 1) will never result in arithmetic overflow. */
                    AspIntegerResult integerResult = AspMultiplyIntegers
                        (count - 1, stepValue, &initialValue);
                    if (integerResult == AspIntegerResult_OK)
                        integerResult = AspAddIntegers
                            (startValue, initialValue, &initialValue);
                    if (integerResult != AspIntegerResult_OK)
                    {
                        result.result =
                            AspTranslateIntegerResult(integerResult);
                        return result;
                    }
                    atEnd = ReversedRangeIteratorAtEnd
                        (initialValue, startValue, endValue, stepValue);
                }
            }
            else
            {
                initialValue = startValue;
                atEnd = AspIsValueAtRangeEnd
                    (startValue, endValue, stepValue, bounded);
            }

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
                AspDataSetInteger(value, initialValue);
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
                (engine, iterable, 0, !reversed);
            if (startResult.result != AspRunResult_OK)
            {
                result.result = startResult.result;
                break;
            }
            member = startResult.element;

            if (member != 0 && iterableType == DataType_String && reversed)
            {
                AspDataSetIteratorStringIndex
                    (iterator,
                     AspDataGetStringFragmentSize(startResult.value) - 1U);
            }

            break;
        }

        case DataType_Ellipsis:
        case DataType_Module:
            iterable =
                iterableType == DataType_Module ?
                AspValueEntry
                    (engine, AspDataGetModuleNamespaceIndex(iterable)) :
                engine->localNamespace;

            /* Fall through... */

        case DataType_Set:
        case DataType_Dictionary:
        {
            AspTreeResult startResult = AspTreeNext
                (engine, iterable, 0, !reversed);
            if (startResult.result != AspRunResult_OK)
            {
                result.result = startResult.result;
                break;
            }
            member = startResult.node;

            break;
        }

        case DataType_ForwardIterator:
        case DataType_ReverseIterator:
        {
            /* Copy the iterator. */
            AspDataSetIteratorIterableIndex
                (iterator, AspIndex(engine, iterable));
            member = AspValueEntry
                (engine, AspDataGetIteratorMemberIndex(oldIterator));
            bool needsCleanup = AspDataGetIteratorMemberNeedsCleanup
                (oldIterator);
            AspDataSetIteratorMemberNeedsCleanup(iterator, needsCleanup);
            if (needsCleanup)
                AspRef(engine, member);
            reversed = (iterableType == DataType_ReverseIterator) != reversed;
            if (AspDataGetType(iterable) == DataType_String)
                AspDataSetIteratorStringIndex
                    (iterator, AspDataGetIteratorStringIndex(oldIterator));
        }
    }
    AspDataSetIteratorMemberIndex(iterator, AspIndex(engine, member));
    AspDataSetType
        (iterator,
         reversed ? DataType_ReverseIterator : DataType_ForwardIterator);

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

    if (!AspIsIterator(iterator))
        return AspRunResult_UnexpectedType;

    /* Gain access to the underlying iterable. */
    AspDataEntry *iterable = AspValueEntry
        (engine, AspDataGetIteratorIterableIndex(iterator));

    /* Check if the iterator is already at its end. */
    AspDataEntry *member = AspEntry
        (engine, AspDataGetIteratorMemberIndex(iterator));
    if (member == 0)
        return AspRunResult_IteratorAtEnd;

    /* Determine the direction of iteration. */
    bool reversed = AspIsReverseIterator(iterator);

    /* Advance the iterator. */
    uint8_t iterableType = AspDataGetType(iterable);
    switch (iterableType)
    {
        default:
            return AspRunResult_UnexpectedType;

        case DataType_Range:
        {
            if (AspDataGetType(member) != DataType_Integer)
                return AspRunResult_UnexpectedType;

            int32_t startValue, endValue, stepValue;
            bool bounded;
            AspGetRange
                (engine, iterable,
                 &startValue, &endValue, &stepValue, &bounded);
            int32_t newValue;
            AspIntegerResult integerResult =
                (reversed ? AspSubtractIntegers : AspAddIntegers)
                    (AspDataGetInteger(member), stepValue, &newValue);
            if (integerResult != AspIntegerResult_OK)
                return AspTranslateIntegerResult(integerResult);
            AspUnref(engine, member);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
            bool atEnd = reversed ?
                ReversedRangeIteratorAtEnd
                    (newValue, startValue, endValue, stepValue) :
                AspIsValueAtRangeEnd
                    (newValue, endValue, stepValue, bounded);
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

            uint8_t stringIndex =
                AspDataGetIteratorStringIndex(iterator);
            if (reversed)
            {
                if (stringIndex > 0)
                {
                    AspDataSetIteratorStringIndex(iterator, stringIndex - 1);
                    break;
                }
            }
            else
            {
                uint8_t fragmentSize =
                    AspDataGetStringFragmentSize(fragment);
                if (stringIndex + 1 < fragmentSize)
                {
                    AspDataSetIteratorStringIndex(iterator, stringIndex + 1);
                    break;
                }
            }

            /* Fall through... */
        }

        case DataType_Tuple:
        case DataType_List:
        {
            if (AspDataGetType(member) != DataType_Element)
                return AspRunResult_UnexpectedType;
            AspSequenceResult nextResult = AspSequenceNext
                (engine, iterable, member, !reversed);
            if (nextResult.result != AspRunResult_OK)
                return nextResult.result;
            member = nextResult.element;

            if (member != 0 && iterableType == DataType_String)
            {
                AspDataSetIteratorStringIndex
                    (iterator,
                     !reversed ? 0 :
                     AspDataGetStringFragmentSize(nextResult.value) - 1U);
            }

            break;
        }

        case DataType_Ellipsis:
        case DataType_Module:
            iterable =
                iterableType == DataType_Module ?
                AspValueEntry
                    (engine, AspDataGetModuleNamespaceIndex(iterable)) :
                engine->localNamespace;

            /* Fall through... */

        case DataType_Set:
        case DataType_Dictionary:
        {
            uint8_t memberType = AspDataGetType(member);
            if (memberType != DataType_SetNode &&
                memberType != DataType_DictionaryNode &&
                memberType != DataType_NamespaceNode)
                return AspRunResult_UnexpectedType;
            AspTreeResult nextResult = AspTreeNext
                (engine, iterable, member, !reversed);
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

    if (!AspIsIterator(iterator))
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
    uint8_t iterableType = AspDataGetType(iterable);
    AspDataEntry *value = 0;
    switch (iterableType)
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
            const uint8_t *stringData = (const uint8_t *)
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

        case DataType_Ellipsis:
        case DataType_Module:
            iterable =
                iterableType == DataType_Module ?
                AspValueEntry
                    (engine, AspDataGetModuleNamespaceIndex(iterable)) :
                engine->localNamespace;

            /* Fall through... */

        case DataType_Dictionary:
        {
            AspDataEntry *key;
            if (AspDataGetType(iterable) == DataType_Namespace)
            {
                int32_t symbol = AspDataGetNamespaceNodeSymbol(member);
                key = AspAllocEntry(engine, DataType_Symbol);
                if (key == 0)
                {
                    result.result = AspRunResult_OutOfDataMemory;
                    break;
                }
                AspDataSetSymbol(key, symbol);
            }
            else
            {
                key = AspValueEntry
                    (engine, AspDataGetTreeNodeKeyIndex(member));
            }
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
            if (AspDataGetType(iterable) == DataType_Namespace)
                AspUnref(engine, key);
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

static bool ReversedRangeIteratorAtEnd
    (int32_t testValue,
     int32_t startValue, int32_t endValue, int32_t stepValue)
{
    return
        stepValue == 0 ? testValue == endValue :
        stepValue < 0 ? testValue > startValue : testValue < startValue;
}
