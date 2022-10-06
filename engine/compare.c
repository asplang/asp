/*
 * Asp engine object comparison implementation.
 */

#include "compare.h"
#include "range.h"

int AspCompare
    (AspEngine *engine,
     const AspDataEntry *leftEntry, const AspDataEntry *rightEntry)
{
    AspAssert
        (engine, leftEntry != 0 && AspIsObject(leftEntry));
    AspRunResult assertResult = AspAssert
        (engine, rightEntry != 0 && AspIsObject(rightEntry));
    if (assertResult != AspRunResult_OK)
        return 0;

    /* Determine whether objects are of the same type. */
    uint8_t
        leftType = AspDataGetType(leftEntry),
        rightType = AspDataGetType(rightEntry);
    if (leftType != rightType)
    {
        /* Compare objects as numeric if possible. Otherwise, compare
           their types. */
        if (AspIsNumeric(leftEntry) && AspIsNumeric(rightEntry))
        {
            if (leftType != DataType_Float && rightType != DataType_Float)
            {
                int32_t leftValue, rightValue;
                AspIntegerValue(leftEntry, &leftValue);
                AspIntegerValue(rightEntry, &rightValue);
                return
                    leftValue == rightValue ?
                    leftType < rightType ? -1 : 1 :
                    leftValue < rightValue ? -1 : 1;
            }
            else
            {
                double leftValue, rightValue;
                AspFloatValue(leftEntry, &leftValue);
                AspFloatValue(rightEntry, &rightValue);
                return
                    leftValue == rightValue ?
                    leftType < rightType ? -1 : 1 :
                    leftValue < rightValue ? -1 : 1;
            }
        }
        else
            return leftType < rightType ? -1 : 1;
    }

    /* Compare objects of the same type. */
    DataType type = (DataType)leftType;
    switch (type)
    {
        default:
            AspAssert(engine, false);
            return 0;

        case DataType_None:
        case DataType_Ellipsis:
            return 0;

        case DataType_Boolean:
        {
            bool
                leftValue = AspDataGetBoolean(leftEntry),
                rightValue = AspDataGetBoolean(rightEntry);
            return
                leftValue == rightValue ? 0 :
                leftValue < rightValue ? -1 : 1;
        }

        case DataType_Integer:
        {
            int32_t
                leftValue = AspDataGetInteger(leftEntry),
                rightValue = AspDataGetInteger(rightEntry);
            return
                leftValue == rightValue ? 0 :
                leftValue < rightValue ? -1 : 1;
        }

        case DataType_Float:
        {
            double
                leftValue = AspDataGetFloat(leftEntry),
                rightValue = AspDataGetFloat(rightEntry);
            return
                leftValue == rightValue ? 0 :
                leftValue < rightValue ? -1 : 1;
        }

        case DataType_Range:
        {
            int32_t leftStart, leftEnd, leftStep;
            int32_t rightStart, rightEnd, rightStep;
            AspGetRange
                (engine, leftEntry,
                 &leftStart, &leftEnd, &leftStep);
            AspGetRange
                (engine, rightEntry,
                 &rightStart, &rightEnd, &rightStep);
            return
                leftStart == rightStart ?
                leftEnd == rightEnd ?
                leftStep == rightStep ? 0 :
                leftStep < rightStep ? -1 : 1 :
                leftEnd < rightEnd ? -1 : 1 :
                leftStart < rightStart ? -1 : 1;
        }

        case DataType_String:
        {
            uint32_t leftCount = AspDataGetSequenceCount(leftEntry);
            uint32_t rightCount = AspDataGetSequenceCount(rightEntry);
            uint32_t count = leftCount < rightCount ? leftCount : rightCount;
            for (uint32_t i = 0; i < count; i++)
            {
                char leftChar = AspStringElement(engine, leftEntry, i);
                char rightChar = AspStringElement(engine, rightEntry, i);
                if (leftChar != rightChar)
                    return leftChar < rightChar ? -1 : 1;
            }
            return
                leftCount == rightCount ? 0 :
                leftCount < rightCount ? -1 : 1;
        }

        case DataType_Tuple:
        case DataType_List:
        case DataType_Set:
        case DataType_Dictionary:
        case DataType_Iterator:
        {
            /* TODO: Implement comparisons for each type.
               Hint: Make use of the *ComparsionOperation routines in
               operation.c. */
            AspAssert(engine, false);
            /* For now, just sort by "address". Note that this will not do
               because we need to be able to compare constructed keys with
               keys already in a tree. Therefore, this has to be a value
               comparison. */
            uint32_t
                leftIndex = AspIndex(engine, leftEntry),
                rightIndex = AspIndex(engine, rightEntry);
            return
                leftIndex == rightIndex ? 0 :
                leftIndex < rightIndex ? -1 : 1;
        }

        case DataType_Function:
        {
            uint32_t leftModule = AspDataGetFunctionModuleIndex(leftEntry);
            uint32_t rightModule = AspDataGetFunctionModuleIndex(rightEntry);
            int32_t leftSymbol = AspDataGetFunctionSymbol(leftEntry);
            int32_t rightSymbol = AspDataGetFunctionSymbol(rightEntry);
            return
                leftModule == rightModule ?
                leftSymbol == rightSymbol ? 0 :
                leftSymbol < rightSymbol ? -1 : 1 :
                leftModule < rightModule ? -1 : 1;
        }

        case DataType_Module:
        {
            uint32_t leftValue = AspDataGetModuleCodeAddress(leftEntry);
            uint32_t rightValue = AspDataGetModuleCodeAddress(rightEntry);
            return
                leftValue == rightValue ? 0 :
                leftValue < rightValue ? -1 : 1;
        }

        case DataType_Type:
        {
            uint8_t leftValue = AspDataGetTypeValue(leftEntry);
            uint8_t rightValue = AspDataGetTypeValue(rightEntry);
            return
                leftValue == rightValue ? 0 :
                leftValue < rightValue ? -1 : 1;
        }
    }
}
