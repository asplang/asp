/*
 * Asp engine object comparison implementation.
 */

#include "compare.h"
#include "stack.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include <math.h>
#include <string.h>

static int CompareFloats(double, double, AspCompareType, bool *nanDetected);
static int CompareIterators(const AspDataEntry *, const AspDataEntry *);

AspRunResult AspCompare
    (AspEngine *engine,
     const AspDataEntry *leftEntry, const AspDataEntry *rightEntry,
     AspCompareType compareType, int *result, bool *nanDetected)
{
    AspAssert
        (engine, leftEntry != 0 && AspIsObject(leftEntry));
    AspRunResult assertResult = AspAssert
        (engine, rightEntry != 0 && AspIsObject(rightEntry));
    if (assertResult != AspRunResult_OK)
        return assertResult;
    bool localNanDetected = false;

    /* Avoid recursion by using the engine's stack. */
    AspDataEntry *startStackTop = engine->stackTop;
    int comparison = 0;
    AspDataEntry *leftNext = 0, *rightNext = 0;
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        /* Determine whether objects are of the same type. */
        uint8_t
            leftType = AspDataGetType(leftEntry),
            rightType = AspDataGetType(rightEntry);
        if (leftType != rightType)
        {
            if (compareType == AspCompareType_Key ||
                compareType == AspCompareType_Order)
                comparison = leftType < rightType ? -1 : 1;
            else if ((leftType == DataType_Boolean ||
                      leftType == DataType_Integer ||
                      leftType == DataType_Float) &&
                     (rightType == DataType_Boolean ||
                      rightType == DataType_Integer ||
                      rightType == DataType_Float))
            {
                /* Perform a numeric comparison, determining the types
                   of the operands, and therefore how to compare them. */
                int32_t leftInt = 0, rightInt = 0;
                if (leftType == DataType_Boolean)
                    leftInt = (uint32_t)AspDataGetBoolean(leftEntry);
                else if (leftType == DataType_Integer)
                    leftInt = (uint32_t)AspDataGetInteger(leftEntry);
                if (rightType == DataType_Boolean)
                    rightInt = (uint32_t)AspDataGetBoolean(rightEntry);
                else if (rightType == DataType_Integer)
                    rightInt = (uint32_t)AspDataGetInteger(rightEntry);
                bool compareInts = true;
                double leftFloat = 0.0, rightFloat = 0.0;
                if (leftType == DataType_Float || rightType == DataType_Float)
                {
                    compareInts = false;
                    leftFloat = leftType == DataType_Float ?
                        AspDataGetFloat(leftEntry) : (double)leftInt;
                    rightFloat = rightType == DataType_Float ?
                        AspDataGetFloat(rightEntry) : (double)rightInt;
                }

                comparison = compareInts ?
                    leftInt == rightInt ? 0 :
                    leftInt < rightInt ? -1 : +1 :
                    CompareFloats
                        (leftFloat, rightFloat,
                         compareType, &localNanDetected);
            }
            else if (compareType == AspCompareType_Equality)
            {
                if ((leftType == DataType_ForwardIterator ||
                     rightType == DataType_ForwardIterator) &&
                    (leftType == DataType_ReverseIterator ||
                     rightType == DataType_ReverseIterator))
                {
                    /* Compare iterators of different types (forward and
                       reverse). They are only ever tested for equality. */
                    comparison = CompareIterators(leftEntry, rightEntry);
                }
                else
                    comparison = 1;
            }
            else
                return AspRunResult_UnexpectedType;
        }
        else
        {
            /* Reject invalid comparisons. */
            DataType type = (DataType)leftType;
            switch (compareType)
            {
                default:
                    break;

                case AspCompareType_Relational:
                    if (type == DataType_Symbol ||
                        type == DataType_Range ||
                        type == DataType_Set ||
                        type == DataType_Dictionary ||
                        type == DataType_ForwardIterator ||
                        type == DataType_ReverseIterator ||
                        type == DataType_Function ||
                        type == DataType_Module ||
                        type == DataType_AppIntegerObject ||
                        type == DataType_AppPointerObject ||
                        type == DataType_Type)
                        return AspRunResult_UnexpectedType;
                    break;

                case AspCompareType_Key:
                    if (type == DataType_List ||
                        type == DataType_Set ||
                        type == DataType_Dictionary ||
                        type == DataType_ForwardIterator ||
                        type == DataType_ReverseIterator)
                        return AspRunResult_UnexpectedType;
                    break;
            }

            /* Compare objects, unless they are the same object. */
            if (leftEntry != rightEntry)
            {
                /* Compare objects of the same type. */
                switch (type)
                {
                    default:
                        return AspAssert(engine, false);

                    case DataType_None:
                    case DataType_Ellipsis:
                        break;

                    case DataType_Boolean:
                    {
                        bool
                            leftValue = AspDataGetBoolean(leftEntry),
                            rightValue = AspDataGetBoolean(rightEntry);
                        if (leftValue != rightValue)
                            comparison = leftValue < rightValue ? -1 : 1;
                        break;
                    }

                    case DataType_Integer:
                    {
                        int32_t
                            leftValue = AspDataGetInteger(leftEntry),
                            rightValue = AspDataGetInteger(rightEntry);
                        if (leftValue != rightValue)
                            comparison = leftValue < rightValue ? -1 : 1;
                        break;
                    }

                    case DataType_Float:
                    {
                        double
                            leftValue = AspDataGetFloat(leftEntry),
                            rightValue = AspDataGetFloat(rightEntry);
                        comparison = CompareFloats
                            (leftValue, rightValue,
                             compareType, &localNanDetected);
                        break;
                    }

                    case DataType_Symbol:
                    {
                        int32_t
                            leftValue = AspDataGetSymbol(leftEntry),
                            rightValue = AspDataGetSymbol(rightEntry);
                        if (leftValue != rightValue)
                            comparison = leftValue < rightValue ? -1 : 1;
                        break;
                    }

                    case DataType_Range:
                    {
                        int32_t
                            leftStart, leftEnd, leftStep,
                            rightStart, rightEnd, rightStep;
                        bool leftBounded, rightBounded;
                        AspGetRange
                            (engine, leftEntry,
                             &leftStart, &leftEnd, &leftStep, &leftBounded);
                        AspGetRange
                            (engine, rightEntry,
                             &rightStart, &rightEnd, &rightStep, &rightBounded);
                        comparison =
                            leftBounded == rightBounded ?
                            leftStart == rightStart ?
                            leftEnd == rightEnd ?
                            leftStep == rightStep ? 0 :
                            leftStep < rightStep ? -1 : 1 :
                            leftEnd < rightEnd ? -1 : 1 :
                            leftStart < rightStart ? -1 : 1 :
                            leftBounded > rightBounded ? -1 : 1;
                        break;
                    }

                    case DataType_String:
                    {
                        int32_t
                            leftCount = AspDataGetSequenceCount(leftEntry),
                            rightCount = AspDataGetSequenceCount(rightEntry);
                        int32_t shortCount =
                            leftCount < rightCount ? leftCount : rightCount;
                        for (int32_t i = 0; i < shortCount; i++)
                        {
                            char
                                leftChar = AspStringElement
                                    (engine, leftEntry, i),
                                rightChar = AspStringElement
                                    (engine, rightEntry, i);
                            if (leftChar != rightChar)
                            {
                                comparison = leftChar < rightChar ? -1 : 1;
                                break;
                            }
                        }
                        if (comparison == 0 && leftCount != rightCount)
                            comparison = leftCount < rightCount ? -1 : 1;

                        break;
                    }

                    case DataType_Tuple:
                    case DataType_List:
                    {
                        /* For key or order comparison, compare counts first
                           for efficiency. */
                        if (compareType == AspCompareType_Key ||
                            compareType == AspCompareType_Order)
                        {
                            int32_t
                                leftCount = AspDataGetSequenceCount
                                    (leftEntry),
                                rightCount = AspDataGetSequenceCount
                                    (rightEntry);
                            if (leftCount != rightCount)
                            {
                                comparison = leftCount < rightCount ? -1 : 1;
                                break;
                            }
                        }

                        /* Examine the next element of each sequence. */
                        AspDataEntry
                            *mutableLeftEntry = (AspDataEntry *)leftEntry,
                            *mutableRightEntry = (AspDataEntry *)rightEntry;
                        AspSequenceResult
                            leftResult = AspSequenceNext
                                (engine, mutableLeftEntry, leftNext, true),
                            rightResult = AspSequenceNext
                                (engine, mutableRightEntry, rightNext, true);
                        leftNext = leftResult.element;
                        rightNext = rightResult.element;
                        if (leftNext == 0 || rightNext == 0)
                        {
                            if (leftNext != 0 || rightNext != 0)
                                comparison = leftNext == 0 ? -1 : 1;
                            break;
                        }

                        /* Save state and defer element comparison to the
                           next iteration. */
                        AspDataEntry *entriesStackEntry = AspPushNoUse
                            (engine, mutableLeftEntry);
                        AspDataEntry *nextsStackEntry = AspPushNoUse
                            (engine, leftNext);
                        AspDataEntry *valuesStackEntry = AspPushNoUse
                            (engine, leftResult.value);
                        if (entriesStackEntry == 0 ||
                            nextsStackEntry == 0 ||
                            valuesStackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetStackEntryHasValue2
                            (entriesStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (entriesStackEntry,
                             AspIndex(engine, mutableRightEntry));
                        AspDataSetStackEntryHasValue2
                            (nextsStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (nextsStackEntry, AspIndex(engine, rightNext));
                        AspDataSetStackEntryHasValue2
                            (valuesStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (valuesStackEntry,
                             AspIndex(engine, rightResult.value));

                        break;
                    }

                    case DataType_Set:
                    case DataType_Dictionary:
                    {
                        /* For order comparison, compare counts first for
                           efficiency. */
                        if (compareType == AspCompareType_Order)
                        {
                            int32_t
                                leftCount = AspDataGetTreeCount(leftEntry),
                                rightCount = AspDataGetTreeCount(rightEntry);
                            if (leftCount != rightCount)
                            {
                                comparison = leftCount < rightCount ? -1 : 1;
                                break;
                            }
                        }

                        /* Examine the next node of each tree. */
                        AspDataEntry
                            *mutableLeftEntry = (AspDataEntry *)leftEntry,
                            *mutableRightEntry = (AspDataEntry *)rightEntry;
                        AspTreeResult
                            leftResult = AspTreeNext
                                (engine, mutableLeftEntry, leftNext, true),
                            rightResult = AspTreeNext
                                (engine, mutableRightEntry, rightNext, true);
                        leftNext = leftResult.node;
                        rightNext = rightResult.node;
                        if (leftNext == 0 || rightNext == 0)
                        {
                            if (leftNext != 0 || rightNext != 0)
                                comparison = leftNext == 0 ? -1 : 1;
                            break;
                        }

                        /* Save state and defer member comparison to the
                           next iteration. */
                        AspDataEntry *entriesStackEntry = AspPushNoUse
                            (engine, mutableLeftEntry);
                        AspDataEntry *nextsStackEntry = AspPushNoUse
                            (engine, leftNext);
                        AspDataEntry *leftKeyStackEntry = AspPushNoUse
                            (engine, leftResult.key);
                        if (entriesStackEntry == 0 ||
                            nextsStackEntry == 0 ||
                            leftKeyStackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetStackEntryHasValue2
                            (entriesStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (entriesStackEntry,
                             AspIndex(engine, mutableRightEntry));
                        AspDataSetStackEntryHasValue2
                            (nextsStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (nextsStackEntry, AspIndex(engine, rightNext));
                        AspDataSetStackEntryHasValue2
                            (leftKeyStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftKeyStackEntry,
                             AspIndex(engine, rightResult.key));
                        if (type == DataType_Dictionary)
                        {
                            AspDataEntry *valuesStackEntry = AspPushNoUse
                                (engine, leftResult.value);
                            if (valuesStackEntry == 0)
                                return AspRunResult_OutOfDataMemory;
                            AspDataSetStackEntryHasValue2
                                (valuesStackEntry, true);
                            AspDataSetStackEntryValue2Index
                                (valuesStackEntry,
                                 AspIndex(engine, rightResult.value));
                        }

                        break;
                    }

                    case DataType_ForwardIterator:
                    case DataType_ReverseIterator:
                    {
                        /* Iterators are only ever tested for equality. */
                        comparison = CompareIterators(leftEntry, rightEntry);
                        break;
                    }

                    case DataType_Function:
                    {
                        bool
                            leftIsApp = AspDataGetFunctionIsApp(leftEntry),
                            rightIsApp = AspDataGetFunctionIsApp(rightEntry);
                        if (leftIsApp != rightIsApp)
                            comparison = leftIsApp < rightIsApp ? -1 : 1;
                        else if (leftIsApp)
                        {
                            int32_t
                                leftSymbol = AspDataGetFunctionSymbol
                                    (leftEntry),
                                rightSymbol = AspDataGetFunctionSymbol
                                    (rightEntry);
                            comparison =
                                leftSymbol == rightSymbol ? 0 :
                                leftSymbol < rightSymbol ? -1 : 1;
                        }
                        else
                        {
                            uint32_t
                                leftAddress = AspDataGetFunctionCodeAddress
                                    (leftEntry),
                                rightAddress = AspDataGetFunctionCodeAddress
                                    (rightEntry);
                            comparison =
                                leftAddress == rightAddress ? 0 :
                                leftAddress < rightAddress ? -1 : 1;
                        }

                        break;
                    }

                    case DataType_Module:
                    {
                        uint32_t
                            leftValue = AspDataGetModuleCodeAddress
                                (leftEntry),
                            rightValue = AspDataGetModuleCodeAddress
                                (rightEntry);
                        comparison =
                            leftValue == rightValue ? 0 :
                            leftValue < rightValue ? -1 : 1;
                        break;
                    }

                    case DataType_AppIntegerObject:
                    {
                        int16_t
                            leftType = AspDataGetAppObjectType(leftEntry),
                            rightType = AspDataGetAppObjectType(rightEntry);
                        int32_t
                            leftValue = AspDataGetAppIntegerObjectValue
                                (leftEntry),
                            rightValue = AspDataGetAppIntegerObjectValue
                                (rightEntry);
                        comparison =
                            leftType == rightType ?
                            leftValue == rightValue ? 0 :
                            leftValue < rightValue ? -1 : 1 :
                            leftType < rightType ? -1 : 1;
                        break;
                    }

                    case DataType_AppPointerObject:
                    {
                        int16_t
                            leftType = AspDataGetAppObjectType(leftEntry),
                            rightType = AspDataGetAppObjectType(rightEntry);
                        void
                            *leftValue = AspDataGetAppPointerObjectValue
                                (leftEntry),
                            *rightValue = AspDataGetAppPointerObjectValue
                                (rightEntry);
                        comparison =
                            leftType == rightType ?
                            leftValue == rightValue ? 0 :
                            leftValue < rightValue ? -1 : 1 :
                            leftType < rightType ? -1 : 1;
                        break;
                    }

                    case DataType_Type:
                    {
                        uint8_t
                            leftValue = AspDataGetTypeValue(leftEntry),
                            rightValue = AspDataGetTypeValue(rightEntry);
                        comparison =
                            leftValue == rightValue ? 0 :
                            leftValue < rightValue ? -1 : 1;
                        break;
                    }
                }
            }
            else if (type == DataType_Float)
            {
                /* Compare floating-point objects even if they are the same
                   object in order to handle the special case of NaNs, which
                   yield nonintuitive, albeit standardized, results. */
                double
                    leftValue = AspDataGetFloat(leftEntry),
                    rightValue = AspDataGetFloat(rightEntry);
                comparison = CompareFloats
                    (leftValue, rightValue,
                     compareType, &localNanDetected);
            }
        }

        /* Check if there's more to do. */
        if (comparison != 0 || localNanDetected ||
            engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        /* Fetch the next pair of items from the stack. */
        rightEntry = AspTopValue2(engine);
        assertResult = AspAssert(engine, rightEntry != 0);
        if (assertResult != AspRunResult_OK)
            return assertResult;
        leftEntry = AspTopValue(engine);
        AspPopNoErase(engine);

        /* Fetch more items if applicable. */
        switch (AspDataGetType(leftEntry))
        {
            default:
                rightNext = leftNext = 0;
                break;

            case DataType_Element:
            case DataType_SetNode:
            case DataType_DictionaryNode:
            {
                /* The next pair is part of an iteration, so fetch the pair
                   of containers as well. */
                rightNext = (AspDataEntry *)rightEntry;
                leftNext = (AspDataEntry *)leftEntry;
                rightEntry = AspTopValue2(engine);
                assertResult = AspAssert(engine, rightEntry != 0);
                if (assertResult != AspRunResult_OK)
                    return assertResult;
                leftEntry = AspTopValue(engine);
                AspPopNoErase(engine);
                break;
            }
        }
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

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
            return AspRunResult_CycleDetected;
    }

    *result = comparison;
    if (nanDetected != 0)
        *nanDetected = localNanDetected;
    return AspRunResult_OK;
}

static int CompareFloats
    (double leftValue, double rightValue,
     AspCompareType compareType, bool *nanDetected)
{
    /* Perform a standard comparison when possible. Note that when NaNs are
       involved, the result will be nonintuitive, but standardized. */
    bool leftIsNaN = isnan(leftValue), rightIsNaN = isnan(rightValue);
    bool localNanDetected = leftIsNaN || rightIsNaN;
    if ((compareType != AspCompareType_Key &&
         compareType != AspCompareType_Order) ||
        !localNanDetected)
    {
        /* Update whether a NaN has been detected, if applicable. */
        *nanDetected = *nanDetected || localNanDetected;
        return
            leftValue == rightValue ? 0 :
            leftValue < rightValue ? -1 : +1;
    }

    /* Handle NaNs in key comparisons, which must yield predictable results.
       First, place all NaNs prior to non-NaNs in the sort order. */
    if (leftIsNaN != rightIsNaN)
        return leftIsNaN ? -1 : +1;

    /* When comparing two NaNs, perform a comparison of their binary
       representation. Note that a good optimizer will eliminate all but one
       of the code paths below. */
    #ifdef UINT64_MAX
    if (sizeof(uint64_t) == sizeof(double))
    {
        /* Compare the bit patterns using the matching sized integer. */
        uint64_t leftBits = *(const uint64_t *)&leftValue;
        uint64_t rightBits = *(const uint64_t *)&rightValue;
        return
            leftBits == rightBits ? 0 :
            leftBits < rightBits ? -1 : +1;
    }
    else
    #endif
    {
        /* Compare as bytes. */
        static const uint16_t word = 1;
        bool be = *(const char *)&word == 0;
        uint8_t *leftBytes = (uint8_t *)&leftValue;
        uint8_t *rightBytes = (uint8_t *)&rightValue;
        if (!be)
        {
            /* Put values into big endian order. */
            for (unsigned i = 0; i < sizeof(double) / 2; i++)
            {
                unsigned j = sizeof(double) - i - 1;
                leftBytes[i] ^= leftBytes[j];
                leftBytes[j] ^= leftBytes[i];
                leftBytes[i] ^= leftBytes[j];
                rightBytes[i] ^= rightBytes[j];
                rightBytes[j] ^= rightBytes[i];
                rightBytes[i] ^= rightBytes[j];
            }
        }
        return memcmp(leftBytes, rightBytes, sizeof(double));
    }
}

static int CompareIterators
    (const AspDataEntry *leftEntry, const AspDataEntry *rightEntry)
{
    return
        AspDataGetIteratorIterableIndex(leftEntry) ==
        AspDataGetIteratorIterableIndex(rightEntry) &&
        AspDataGetIteratorMemberIndex(leftEntry) ==
        AspDataGetIteratorMemberIndex(rightEntry) &&
        AspDataGetIteratorStringIndex(leftEntry) ==
        AspDataGetIteratorStringIndex(rightEntry) ?
        0 : 1;
}
