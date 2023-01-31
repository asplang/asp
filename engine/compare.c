/*
 * Asp engine object comparison implementation.
 */

#include "compare.h"
#include "stack.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"

AspRunResult AspCompare
    (AspEngine *engine,
     const AspDataEntry *leftEntry, const AspDataEntry *rightEntry,
     AspCompareType compareType, int *result)
{
    AspAssert
        (engine, leftEntry != 0 && AspIsObject(leftEntry));
    AspRunResult assertResult = AspAssert
        (engine, rightEntry != 0 && AspIsObject(rightEntry));
    if (assertResult != AspRunResult_OK)
        return assertResult;

    /* Avoid recursion by using the engine's stack. */
    AspDataEntry *startStackTop = engine->stackTop;
    int comparison = 0;
    AspDataEntry *leftNext = 0, *rightNext = 0;
    while (true)
    {
        /* Determine whether objects are of the same type. */
        uint8_t
            leftType = AspDataGetType(leftEntry),
            rightType = AspDataGetType(rightEntry);
        if (leftType != rightType)
        {
            if (compareType == AspCompareType_Key)
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
                bool compareInts = false;
                double leftFloat = 0.0, rightFloat = 0.0;
                if (leftType == DataType_Float || rightType == DataType_Float)
                {
                    compareInts = false;
                    if (leftType != DataType_Float)
                        leftFloat = (double)leftInt;
                    else
                        leftFloat = (uint32_t)AspDataGetFloat(leftEntry);
                    if (rightType != DataType_Float)
                        rightFloat = (double)rightInt;
                    else
                        rightFloat = (uint32_t)AspDataGetFloat(rightEntry);
                }

                comparison = compareInts ?
                    leftInt == rightInt ? 0 :
                        leftInt < rightInt ? -1 : +1 :
                    leftFloat == rightFloat ? 0 :
                        leftFloat < rightFloat ? -1 : +1;
            }
            else if (compareType == AspCompareType_Equality)
                comparison = 1;
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
                    if (type == DataType_Range ||
                        type == DataType_Set ||
                        type == DataType_Dictionary ||
                        type == DataType_Iterator ||
                        type == DataType_Function ||
                        type == DataType_Module ||
                        type == DataType_Type)
                        return AspRunResult_UnexpectedType;
                    break;

                case AspCompareType_Key:
                    if (type == DataType_List ||
                        type == DataType_Set ||
                        type == DataType_Dictionary ||
                        type == DataType_Iterator)
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
                        if (leftValue != rightValue)
                            comparison = leftValue < rightValue ? -1 : 1;
                        break;
                    }

                    case DataType_Range:
                    {
                        int32_t
                            leftStart, leftEnd, leftStep,
                            rightStart, rightEnd, rightStep;
                        AspGetRange
                            (engine, leftEntry,
                             &leftStart, &leftEnd, &leftStep);
                        AspGetRange
                            (engine, rightEntry,
                             &rightStart, &rightEnd, &rightStep);
                        comparison =
                            leftStart == rightStart ?
                            leftEnd == rightEnd ?
                            leftStep == rightStep ? 0 :
                            leftStep < rightStep ? -1 : 1 :
                            leftEnd < rightEnd ? -1 : 1 :
                            leftStart < rightStart ? -1 : 1;
                        break;
                    }

                    case DataType_String:
                    {
                        uint32_t
                            leftCount = AspDataGetSequenceCount(leftEntry),
                            rightCount = AspDataGetSequenceCount(rightEntry);
                        uint32_t shortCount =
                            leftCount < rightCount ? leftCount : rightCount;
                        for (uint32_t i = 0; i < shortCount; i++)
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
                        /* For key comparison, compare counts first for
                           efficiency. */
                        if (compareType == AspCompareType_Key)
                        {
                            uint32_t
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
                                (engine, mutableLeftEntry, leftNext),
                            rightResult = AspSequenceNext
                                (engine, mutableRightEntry, rightNext);
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
                        AspDataEntry *leftEntryStackEntry = AspPushNoUse
                            (engine, mutableLeftEntry);
                        AspDataEntry *leftNextStackEntry = AspPushNoUse
                            (engine, leftNext);
                        AspDataEntry *leftValueStackEntry = AspPushNoUse
                            (engine, leftResult.value);
                        if (leftEntryStackEntry == 0 ||
                            leftNextStackEntry == 0 ||
                            leftValueStackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetStackEntryHasValue2
                            (leftEntryStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftEntryStackEntry,
                             AspIndex(engine, mutableRightEntry));
                        AspDataSetStackEntryHasValue2
                            (leftNextStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftNextStackEntry,
                             AspIndex(engine, rightNext));
                        AspDataSetStackEntryHasValue2
                            (leftValueStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftValueStackEntry,
                             AspIndex(engine, rightResult.value));

                        break;
                    }

                    case DataType_Set:
                    case DataType_Dictionary:
                    {
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
                        AspDataEntry *leftEntryStackEntry = AspPushNoUse
                            (engine, mutableLeftEntry);
                        AspDataEntry *leftNextStackEntry = AspPushNoUse
                            (engine, leftNext);
                        AspDataEntry *leftKeyStackEntry = AspPushNoUse
                            (engine, leftResult.key);
                        if (leftEntryStackEntry == 0 ||
                            leftNextStackEntry == 0 ||
                            leftKeyStackEntry == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetStackEntryHasValue2
                            (leftEntryStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftEntryStackEntry,
                             AspIndex(engine, mutableRightEntry));
                        AspDataSetStackEntryHasValue2
                            (leftNextStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftNextStackEntry,
                             AspIndex(engine, rightNext));
                        AspDataSetStackEntryHasValue2
                            (leftKeyStackEntry, true);
                        AspDataSetStackEntryValue2Index
                            (leftKeyStackEntry,
                             AspIndex(engine, rightResult.key));
                        if (type == DataType_Dictionary)
                        {
                            AspDataEntry *leftValueStackEntry = AspPushNoUse
                                (engine, leftResult.value);
                            if (leftValueStackEntry == 0)
                                return AspRunResult_OutOfDataMemory;
                            AspDataSetStackEntryHasValue2
                                (leftValueStackEntry, true);
                            AspDataSetStackEntryValue2Index
                                (leftValueStackEntry,
                                 AspIndex(engine, rightResult.value));
                        }

                        break;
                    }

                    case DataType_Iterator:
                    {
                        /* Iterators are only ever tested for equality. */
                        comparison =
                            AspDataGetIteratorIterableIndex(leftEntry) ==
                            AspDataGetIteratorIterableIndex(rightEntry) &&
                            AspDataGetIteratorMemberIndex(leftEntry) ==
                            AspDataGetIteratorMemberIndex(rightEntry) &&
                            AspDataGetIteratorStringIndex(leftEntry) ==
                            AspDataGetIteratorStringIndex(rightEntry) ?
                            0 : 1;
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
        }

        /* Check if there's more to do. */
        if (comparison != 0 ||
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

    /* Unwind the working stack if necessary. */
    if (engine->runResult == AspRunResult_OK)
        while (engine->stackTop != startStackTop)
            AspPopNoErase(engine);

    *result = comparison;
    return AspRunResult_OK;
}
