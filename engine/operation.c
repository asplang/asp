/*
 * Asp engine operations implementation.
 */

#include "operation.h"
#include "asp-priv.h"
#include "opcode.h"
#include "data.h"
#include "range.h"
#include <math.h>
#include <stdint.h>

static AspOperationResult PerformLogicalBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformBitwiseBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformArithmeticBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformConcatenationBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformExpansionBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformEqualityOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformNumericComparisonOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformSequenceComparisonOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformMembershipOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformObjectEqualityOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);

AspOperationResult AspPerformUnaryOperation
    (AspEngine *engine, uint8_t opCode, AspDataEntry *operand)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    result.result = AspAssert
        (engine, operand != 0 && AspIsObject(operand));
    if (result.result != AspRunResult_OK)
        return result;

    uint8_t operandType = AspDataGetType(operand);
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_LNOT:
            result.value = AspAllocEntry(engine, DataType_Boolean);
            if (result.value == 0)
                break;
            AspDataSetBoolean(result.value, !AspIsTrue(engine, operand));
            break;

        case OpCode_NEG:
            switch (operandType)
            {
                default:
                    result.result = AspRunResult_UnexpectedType;
                    break;

                case DataType_Boolean:
                    result.value = AspAllocEntry(engine, DataType_Integer);
                    if (result.value == 0)
                        break;
                    AspDataSetInteger(result.value,
                        -(int32_t)AspDataGetBoolean(operand));
                    break;

                case DataType_Integer:
                    result.value = AspAllocEntry(engine, DataType_Integer);
                    if (result.value == 0)
                        break;
                    AspDataSetInteger(result.value,
                        -AspDataGetInteger(operand));
                    break;

                case DataType_Float:
                    result.value = AspAllocEntry(engine, DataType_Float);
                    if (result.value == 0)
                        break;
                    AspDataSetFloat(result.value,
                        -AspDataGetFloat(operand));
                    break;
            }

            break;

        case OpCode_NOT:
            switch (operandType)
            {
                default:
                    result.result = AspRunResult_UnexpectedType;
                    break;

                case DataType_Boolean:
                    result.value = AspAllocEntry(engine, DataType_Integer);
                    if (result.value == 0)
                        break;
                    AspDataSetInteger(result.value,
                        ~(uint32_t)AspDataGetBoolean(operand));
                    break;

                case DataType_Integer:
                {
                    result.value = AspAllocEntry(engine, DataType_Integer);
                    if (result.value == 0)
                        break;
                    int32_t operandValue = AspDataGetInteger(operand);
                    uint32_t uOperandValue = *(uint32_t *)&operandValue;
                    uint32_t uResult = ~uOperandValue;
                    AspDataSetInteger(result.value, *(int32_t *)&uResult);
                    break;
                }
            }

            break;
    }

    if (result.result == AspRunResult_OK && result.value == 0)
        result.result = AspRunResult_OutOfDataMemory;
    return result;
}

AspOperationResult AspPerformBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert
        (engine, left != 0 && AspIsObject(left));
    result.result = AspAssert
        (engine, right != 0 && AspIsObject(right));
    if (result.result != AspRunResult_OK)
        return result;

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_LOR:
        case OpCode_LAND:
            result = PerformLogicalBinaryOperation
                (engine, opCode, left, right);
            break;

        case OpCode_OR:
        case OpCode_XOR:
        case OpCode_AND:
        case OpCode_LSH:
        case OpCode_RSH:
            result = PerformBitwiseBinaryOperation
                (engine, opCode, left, right);
            break;

        case OpCode_ADD:
            if ((leftType == DataType_String ||
                 leftType == DataType_Tuple ||
                 leftType == DataType_List) &&
                (rightType == DataType_String ||
                 rightType == DataType_Tuple ||
                 rightType == DataType_List))
            {
                result = PerformConcatenationBinaryOperation
                    (engine, opCode, left, right);
                break;
            }

            /* Fall through... */

        case OpCode_MUL:
            if (opCode == OpCode_MUL &&
                (leftType == DataType_String ||
                 leftType == DataType_Tuple ||
                 leftType == DataType_List) &&
                (rightType == DataType_Boolean ||
                 rightType == DataType_Integer))
            {
                result = PerformExpansionBinaryOperation
                    (engine, opCode, left, right);
                break;
            }

            /* Fall through... */

        case OpCode_SUB:
        case OpCode_DIV:
        case OpCode_FDIV:
        case OpCode_MOD:
        case OpCode_POW:
            if ((leftType == DataType_Boolean ||
                 leftType == DataType_Integer ||
                 leftType == DataType_Float) &&
                (rightType == DataType_Boolean ||
                 rightType == DataType_Integer ||
                 rightType == DataType_Float))
            {
                result = PerformArithmeticBinaryOperation
                    (engine, opCode, left, right);
            }

            break;

        case OpCode_NE:
        case OpCode_EQ:
            result = PerformEqualityOperation
                (engine, opCode, left, right);
            break;

        case OpCode_LT:
        case OpCode_LE:
        case OpCode_GT:
        case OpCode_GE:
            if ((leftType == DataType_Boolean ||
                 leftType == DataType_Integer ||
                 leftType == DataType_Float) &&
                (rightType == DataType_Boolean ||
                 rightType == DataType_Integer ||
                 rightType == DataType_Float))
            {
                result = PerformNumericComparisonOperation
                    (engine, opCode, left, right);
            }
            else if (leftType == rightType &&
                     (leftType == DataType_String ||
                      leftType == DataType_Tuple ||
                      leftType == DataType_List))
            {
                result = PerformSequenceComparisonOperation
                    (engine, opCode, left, right);
            }

            break;

        case OpCode_NIN:
        case OpCode_IN:
            result = PerformMembershipOperation
                (engine, opCode, left, right);
            break;

        case OpCode_NIS:
        case OpCode_IS:
            result = PerformObjectEqualityOperation
                (engine, opCode, left, right);
            break;
    }

    if (result.result == AspRunResult_OK && result.value == 0)
        result.result = AspRunResult_OutOfDataMemory;
    return result;
}

static AspOperationResult PerformLogicalBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_LOR:
            result.value = AspIsTrue(engine, left) ? left : right;
            AspRef(engine, result.value);
            break;

        case OpCode_LAND:
            result.value = !AspIsTrue(engine, left) ? left : right;
            AspRef(engine, result.value);
            break;
    }

    return result;
}

static AspOperationResult PerformBitwiseBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    uint32_t leftBits = 0, rightBits = 0, resultBits = 0;
    if (leftType == DataType_Boolean)
        leftBits = (uint32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftBits = *(uint32_t *)&AspDataGetInteger(left);
    else
        result.result = AspRunResult_UnexpectedType;
    if (rightType == DataType_Boolean)
        rightBits = (uint32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightBits = (uint32_t)AspDataGetInteger(right);
    else
        result.result = AspRunResult_UnexpectedType;

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_OR:
            resultBits = leftBits | rightBits;
            break;

        case OpCode_XOR:
            resultBits = leftBits ^ rightBits;
            break;

        case OpCode_AND:
            resultBits = leftBits & rightBits;
            break;

        case OpCode_LSH:
            resultBits = leftBits << rightBits;
            break;

        case OpCode_RSH:
            resultBits = leftBits >> rightBits;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, DataType_Integer);
        if (result.value != 0)
            AspDataSetInteger(result.value, *(int32_t *)&resultBits);
    }

    return result;
}

static AspOperationResult PerformConcatenationBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_ADD:
            /* TODO: Concatenate two sequences (or strings). */
            result.result = AspRunResult_NotImplemented;
            break;
    }

    return result;
}

static AspOperationResult PerformExpansionBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_MUL:
            /* TODO: Expand the left sequence (or string) a number of times
               indicated by the right operand. */
            result.result = AspRunResult_NotImplemented;
            break;
    }

    return result;
}

static AspOperationResult PerformArithmeticBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert
        (engine,
         leftType == DataType_Boolean ||
         leftType == DataType_Integer ||
         leftType == DataType_Float);
    result.result = AspAssert
        (engine,
         rightType == DataType_Boolean ||
         rightType == DataType_Integer ||
         rightType == DataType_Float);
    if (result.result != AspRunResult_OK)
        return result;

    /* Determine the type and values of the operands, and therefore the
       type of the result. */
    int32_t leftInt = 0, rightInt = 0, intResult = 0;
    double leftFloat = 0.0, rightFloat = 0.0, floatResult = 0.0;
    if (leftType == DataType_Boolean)
        leftInt = (uint32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftInt = (uint32_t)AspDataGetInteger(left);
    if (rightType == DataType_Boolean)
        rightInt = (uint32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightInt = (uint32_t)AspDataGetInteger(right);
    DataType resultType = DataType_Integer;
    if (leftType == DataType_Float || rightType == DataType_Float)
    {
        resultType = DataType_Float;
        if (leftType != DataType_Float)
            leftFloat = (double)leftInt;
        else
            leftFloat = AspDataGetFloat(left);
        if (rightType != DataType_Float)
            rightFloat = (double)rightInt;
        else
            rightFloat = AspDataGetFloat(right);
    }

    if (resultType == DataType_Integer)
    {
        switch (opCode)
        {
            default:
                result.result = AspRunResult_InvalidInstruction;
                break;

            case OpCode_ADD:
                intResult = leftInt + rightInt;
                break;

            case OpCode_SUB:
                intResult = leftInt - rightInt;
                break;

            case OpCode_MUL:
                intResult = leftInt * rightInt;
                break;

            case OpCode_DIV:
                if (rightInt == 0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }
                resultType = DataType_Float;
                floatResult = (double)leftInt / (double)rightInt;
                break;

            case OpCode_FDIV:
                if (rightInt == 0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }
                intResult = leftInt / rightInt;
                break;

            case OpCode_MOD:
            {
                int32_t signedLeft, signedRight, quotient;

                if (rightInt == 0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }

                /* Compute using the quotient rounded toward negative
                   infinity. */
                signedLeft = leftInt < 0 ? -leftInt : leftInt;
                signedRight = rightInt < 0 ? -rightInt : rightInt;
                quotient = signedLeft / signedRight;
                if (leftInt < 0 != rightInt < 0)
                {
                    quotient = -quotient;
                    if (signedLeft % signedRight != 0)
                        quotient--;
                }
                intResult = leftInt - quotient * rightInt;

                break;
            }

            case OpCode_POW:
                resultType = DataType_Float;
                floatResult = pow((double)leftInt, (double)rightInt);
                break;
        }
    }
    else
    {
        switch (opCode)
        {
            default:
                result.result = AspRunResult_InvalidInstruction;
                break;

            case OpCode_ADD:
                floatResult = leftFloat + rightFloat;
                break;

            case OpCode_SUB:
                floatResult = leftFloat - rightFloat;
                break;

            case OpCode_MUL:
                floatResult = leftFloat * rightFloat;
                break;

            case OpCode_DIV:
                if (rightFloat == 0.0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }
                floatResult = leftFloat / rightFloat;
                break;

            case OpCode_FDIV:
                if (rightFloat == 0.0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }
                /* TODO: Research this and implement. */
                result.result = AspRunResult_NotImplemented;
                break;

            case OpCode_MOD:
            {
                if (rightFloat == 0.0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }

                floatResult = (double)
                    (leftFloat - floor(leftFloat / rightFloat) * rightFloat);
                break;
            }

            case OpCode_POW:
                resultType = DataType_Float;
                floatResult = pow(leftFloat, rightFloat);
                break;
        }
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, resultType);
        if (result.value != 0)
        {
            if (resultType == DataType_Integer)
                AspDataSetInteger(result.value, intResult);
            else
                AspDataSetFloat(result.value, floatResult);
        }
    }

    return result;
}

static AspOperationResult PerformEqualityOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    bool isEqual = left == right;
    bool typesEqual = leftType == rightType;
    if (!isEqual && typesEqual)
    {
        switch (leftType)
        {
            default:
                break;

            case DataType_None:
            case DataType_Ellipsis:
                isEqual = true;
                break;

            case DataType_Boolean:
                isEqual =
                    AspDataGetBoolean(left) ==
                    AspDataGetBoolean(right);
                break;

            case DataType_Integer:
                isEqual =
                    AspDataGetInteger(left) ==
                    AspDataGetInteger(right);
                break;

            case DataType_Float:
                isEqual =
                    AspDataGetFloat(left) ==
                    AspDataGetFloat(right);
                break;

            case DataType_Range:
            {
                int32_t leftStartValue, leftEndValue, leftStepValue;
                int32_t rightStartValue, rightEndValue, rightStepValue;
                AspGetRange
                    (engine, left,
                     &leftStartValue, &leftEndValue, &leftStepValue);
                AspGetRange
                    (engine, right,
                     &rightStartValue, &rightEndValue, &rightStepValue);
                isEqual =
                    leftStartValue == rightStartValue &&
                    leftEndValue == rightEndValue &&
                    leftStepValue == rightStepValue;
                break;
            }

            case DataType_String:
                /* TODO: Implement. */
                result.result = AspRunResult_NotImplemented;
                break;

            case DataType_Tuple:
            case DataType_List:
            case DataType_Set:
            case DataType_Dictionary:
                /* TODO: Handle collections "recursively" (but implement
                   with iteration). */
                result.result = AspRunResult_NotImplemented;
                break;

            case DataType_Iterator:
                /* TODO: Implement. */
                result.result = AspRunResult_NotImplemented;
                break;

            case DataType_Type:
                isEqual =
                    AspDataGetTypeValue(left) ==
                    AspDataGetTypeValue(right);
                break;
        }
    }

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_NE:
            resultValue = !isEqual;
            break;

        case OpCode_EQ:
            resultValue = isEqual;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, DataType_Boolean);
        if (result.value != 0)
            AspDataSetBoolean(result.value, resultValue);
    }

    return result;
}

static AspOperationResult PerformNumericComparisonOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert
        (engine,
         leftType == DataType_Boolean ||
         leftType == DataType_Integer ||
         leftType == DataType_Float);
    result.result = AspAssert
        (engine,
         rightType == DataType_Boolean ||
         rightType == DataType_Integer ||
         rightType == DataType_Float);
    if (result.result != AspRunResult_OK)
        return result;

    /* Determine the type and values of the operands, and therefore the
       type of the result. */
    int32_t leftInt = 0, rightInt = 0, intResult = 0;
    double leftFloat = 0.0, rightFloat = 0.0, floatResult = 0.0;
    if (leftType == DataType_Boolean)
        leftInt = (uint32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftInt = (uint32_t)AspDataGetInteger(left);
    if (rightType == DataType_Boolean)
        rightInt = (uint32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightInt = (uint32_t)AspDataGetInteger(right);
    DataType resultType = DataType_Integer;
    if (leftType == DataType_Float || rightType == DataType_Float)
    {
        resultType = DataType_Float;
        if (leftType != DataType_Float)
            leftFloat = (double)leftInt;
        else
            leftFloat = (uint32_t)AspDataGetFloat(left);
        if (rightType != DataType_Float)
            rightFloat = (double)rightInt;
        else
            rightFloat = (uint32_t)AspDataGetFloat(right);
    }

    int comparison = resultType == DataType_Integer ?
        leftInt == rightInt ? 0 : leftInt < rightInt ? -1 : +1 :
        leftFloat == rightFloat ? 0 : leftFloat < rightFloat ? -1 : +1;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_LT:
            resultValue = comparison < 0;
            break;

        case OpCode_LE:
            resultValue = comparison <= 0;
            break;

        case OpCode_GT:
            resultValue = comparison > 0;
            break;

        case OpCode_GE:
            resultValue = comparison >= 0;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, DataType_Boolean);
        if (result.value != 0)
            AspDataSetBoolean(result.value, resultValue);
    }

    return result;
}

static AspOperationResult PerformSequenceComparisonOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert(engine, leftType == rightType);
    result.result = AspAssert
        (engine,
         leftType == DataType_String ||
         leftType == DataType_Tuple ||
         leftType == DataType_List);
    if (result.result != AspRunResult_OK)
        return result;

    /* TODO: Implement. */
    result.result = AspRunResult_NotImplemented;

    return result;
}

static AspOperationResult PerformMembershipOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    /* TODO: Implement. */
    result.result = AspRunResult_NotImplemented;
    bool isIn = false;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_NIN:
            resultValue = !isIn;
            break;

        case OpCode_IN:
            resultValue = isIn;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, DataType_Boolean);
        if (result.value != 0)
            AspDataSetBoolean(result.value, resultValue);
    }

    return result;
}

static AspOperationResult PerformObjectEqualityOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    bool isEqual = left == right;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_NIS:
            resultValue = !isEqual;
            break;

        case OpCode_IS:
            resultValue = isEqual;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspAllocEntry(engine, DataType_Boolean);
        if (result.value != 0)
            AspDataSetBoolean(result.value, resultValue);
    }

    return result;
}

AspOperationResult AspPerformTernaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *condition,
     AspDataEntry *falseValue, AspDataEntry *trueValue)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert
        (engine, condition != 0 && AspIsObject(condition));
    AspAssert
        (engine, falseValue != 0 && AspIsObject(falseValue));
    result.result = AspAssert
        (engine, trueValue != 0 && AspIsObject(trueValue));
    if (result.result != AspRunResult_OK)
        return result;

    bool isConditionTrue = AspIsTrue(engine, condition);

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_COND:
            result.value = isConditionTrue ? trueValue : falseValue;
            AspRef(engine, result.value);
            break;
    }

    return result;
}
