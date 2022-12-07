/*
 * Asp engine operations implementation.
 */

#include "operation.h"
#include "asp-priv.h"
#include "opcode.h"
#include "data.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"
#include <math.h>
#include <stdint.h>

static AspOperationResult PerformBitwiseBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformArithmeticBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformConcatenationBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformRepetitionBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *sequence, AspDataEntry *repeatCount);
static AspOperationResult PerformFormatBinaryOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *sequence, AspDataEntry *repeatCount);
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
static AspOperationResult PerformIdentityOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static bool IsEqual
    (AspEngine *engine, AspDataEntry *left, AspDataEntry *right);

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
            result.value = AspNewBoolean
                (engine, !AspIsTrue(engine, operand));
            if (result.value == 0)
                break;
            break;

        case OpCode_POS:
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
                        (int32_t)AspDataGetBoolean(operand));
                    break;

                case DataType_Integer:
                case DataType_Float:
                    AspRef(engine, operand);
                    result.value = operand;
                    break;
            }

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
        {
            if (opCode == OpCode_MUL)
            {
                bool isLeftSequence =
                    leftType == DataType_String ||
                    leftType == DataType_Tuple ||
                    leftType == DataType_List;
                bool isLeftInteger =
                    leftType == DataType_Boolean ||
                    leftType == DataType_Integer;
                bool isRightSequence =
                    rightType == DataType_String ||
                    rightType == DataType_Tuple ||
                    rightType == DataType_List;
                bool isRightInteger =
                    rightType == DataType_Boolean ||
                    rightType == DataType_Integer;

                if (isLeftSequence && isRightInteger)
                {
                    result = PerformRepetitionBinaryOperation
                        (engine, opCode, left, right);
                    break;
                }
                else if (isLeftInteger && isRightSequence)
                {
                    result = PerformRepetitionBinaryOperation
                        (engine, opCode, right, left);
                    break;
                }
            }

            /* Fall through... */
        }

        case OpCode_MOD:
        {
            if (opCode == OpCode_MOD)
            {
                if (leftType == DataType_String &&
                    rightType == DataType_Tuple)
                {
                    result = PerformFormatBinaryOperation
                        (engine, opCode, right, left);
                    break;
                }
            }

            /* Fall through... */
        }

        case OpCode_SUB:
        case OpCode_DIV:
        case OpCode_FDIV:
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
            else
                result.result = AspRunResult_UnexpectedType;

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
            else
                result.result = AspRunResult_UnexpectedType;

            break;

        case OpCode_NIN:
        case OpCode_IN:
            result = PerformMembershipOperation
                (engine, opCode, left, right);
            break;

        case OpCode_NIS:
        case OpCode_IS:
            result = PerformIdentityOperation
                (engine, opCode, left, right);
            break;
    }

    if (result.result == AspRunResult_OK && result.value == 0)
        result.result = AspRunResult_OutOfDataMemory;
    return result;
}

static AspOperationResult PerformBitwiseBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    int32_t leftValue = 0, rightValue = 0;
    if (leftType == DataType_Boolean)
        leftValue = (int32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftValue = AspDataGetInteger(left);
    else
        result.result = AspRunResult_UnexpectedType;
    uint32_t leftBits = *(uint32_t *)&leftValue;
    if (rightType == DataType_Boolean)
        rightValue = (int32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightValue = AspDataGetInteger(right);
    else
        result.result = AspRunResult_UnexpectedType;
    uint32_t rightBits = *(uint32_t *)&rightValue;

    if (result.result != AspRunResult_OK)
        return result;

    uint32_t resultBits = 0;
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
            if (rightValue < 0)
                result.result = AspRunResult_ValueOutOfRange;
            else
                resultBits = leftBits << rightValue;
            break;

        case OpCode_RSH:
            if (rightValue < 0)
                result.result = AspRunResult_ValueOutOfRange;
            else
            {
                /* Perform sign extension. */
                int32_t resultValue = leftValue >> rightValue;
                resultBits = *(uint32_t *)&resultValue;
            }
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
    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    AspOperationResult result = {AspRunResult_OK, 0};

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_ADD:
        {
            if (leftType != rightType)
            {
                result.result = AspRunResult_UnexpectedType;
                break;
            }
            result.value = AspAllocEntry(engine, leftType);
            if (result.value == 0)
                break;

            AspSequenceResult appendResult = {AspRunResult_OK, 0, 0};
            AspDataEntry *operands[] = {left, right};
            for (unsigned i = 0; i < sizeof operands / sizeof *operands; i++)
            {
                AspSequenceResult nextResult = AspSequenceNext
                    (engine, operands[i], 0);
                for (; nextResult.element != 0;
                     nextResult = AspSequenceNext
                        (engine, operands[i], nextResult.element))
                {
                    AspDataEntry *value = nextResult.value;

                    if (leftType == DataType_String)
                        appendResult.result = AspStringAppendBuffer
                            (engine, result.value,
                             AspDataGetStringFragmentData(value),
                             AspDataGetStringFragmentSize(value));
                    else
                        appendResult = AspSequenceAppend
                            (engine, result.value, value);
                    if (appendResult.result != AspRunResult_OK)
                    {
                        result.result = appendResult.result;
                        break;
                    }
                }
            }

            break;
        }
    }

    if (result.result == AspRunResult_OK && result.value == 0)
        result.result = AspRunResult_OutOfDataMemory;
    return result;
}

static AspOperationResult PerformRepetitionBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *sequence, AspDataEntry *repeatCount)
{
    uint8_t sequenceType = AspDataGetType(sequence);
    uint8_t repeatCountType = AspDataGetType(repeatCount);

    AspOperationResult result = {AspRunResult_OK, 0};

    AspAssert
        (engine,
         sequenceType == DataType_String ||
         sequenceType == DataType_Tuple ||
         sequenceType == DataType_List);
    result.result = AspAssert
        (engine,
         repeatCountType == DataType_Boolean ||
         repeatCountType == DataType_Integer);
    if (result.result != AspRunResult_OK)
        return result;

    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_MUL:
        {
            result.value = AspAllocEntry(engine, sequenceType);
            if (result.value == 0)
                break;

            int32_t repeatCountValue = AspDataGetInteger(repeatCount);
            if (repeatCountValue < 0)
            {
                result.result = AspRunResult_ValueOutOfRange;
                break;
            }

            AspSequenceResult appendResult = {AspRunResult_OK, 0, 0};
            for (int32_t i = 0; i < repeatCountValue; i++)
            {
                AspSequenceResult nextResult = AspSequenceNext
                    (engine, sequence, 0);
                for (; nextResult.element != 0;
                     nextResult = AspSequenceNext
                        (engine, sequence, nextResult.element))
                {
                    AspDataEntry *value = nextResult.value;

                    if (sequenceType == DataType_String)
                        appendResult.result = AspStringAppendBuffer
                            (engine, result.value,
                             AspDataGetStringFragmentData(value),
                             AspDataGetStringFragmentSize(value));
                    else
                        appendResult = AspSequenceAppend
                            (engine, result.value, value);
                    if (appendResult.result != AspRunResult_OK)
                    {
                        result.result = appendResult.result;
                        break;
                    }
                }
            }

            break;
        }
    }

    if (result.result == AspRunResult_OK && result.value == 0)
        result.result = AspRunResult_OutOfDataMemory;
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
    int32_t leftInt = 0, rightInt = 0;
    if (leftType == DataType_Boolean)
        leftInt = (uint32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftInt = (uint32_t)AspDataGetInteger(left);
    if (rightType == DataType_Boolean)
        rightInt = (uint32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightInt = (uint32_t)AspDataGetInteger(right);
    DataType resultType = DataType_Integer;
    double leftFloat = 0.0, rightFloat = 0.0;
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

    int32_t intResult = 0;
    double floatResult = 0.0;
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
                if (leftInt < 0 != rightInt < 0 &&
                    leftInt != intResult * rightInt)
                    intResult--;
                break;

            case OpCode_MOD:
            {
                if (rightInt == 0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }

                /* Compute using the quotient rounded toward negative
                   infinity. */
                int32_t signedLeft = leftInt < 0 ? -leftInt : leftInt;
                int32_t signedRight = rightInt < 0 ? -rightInt : rightInt;
                int32_t quotient = signedLeft / signedRight;
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
                floatResult = floor(leftFloat / rightFloat);
                break;

            case OpCode_MOD:
                if (rightFloat == 0.0)
                {
                    result.result = AspRunResult_DivideByZero;
                    break;
                }
                floatResult =
                    (leftFloat - floor(leftFloat / rightFloat) * rightFloat);
                break;

            case OpCode_POW:
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

static AspOperationResult PerformFormatBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    /* TODO: Implement, either here or in AspLib_format, where it could be
       overridden. */
    AspOperationResult result = {AspRunResult_NotImplemented, 0};
    return result;
}

static AspOperationResult PerformEqualityOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    bool isEqual = IsEqual(engine, left, right);
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
        result.value = AspNewBoolean(engine, resultValue);
        if (result.value == 0)
            result.result = AspRunResult_OutOfDataMemory;
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
    int32_t leftInt = 0, rightInt = 0;
    if (leftType == DataType_Boolean)
        leftInt = (uint32_t)AspDataGetBoolean(left);
    else if (leftType == DataType_Integer)
        leftInt = (uint32_t)AspDataGetInteger(left);
    if (rightType == DataType_Boolean)
        rightInt = (uint32_t)AspDataGetBoolean(right);
    else if (rightType == DataType_Integer)
        rightInt = (uint32_t)AspDataGetInteger(right);
    DataType resultType = DataType_Integer;
    double leftFloat = 0.0, rightFloat = 0.0;
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
        result.value = AspNewBoolean(engine, resultValue);
        if (result.value == 0)
            result.result = AspRunResult_OutOfDataMemory;
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

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);

    bool isIn = false;
    switch (rightType)
    {
        case DataType_String:
        {
            if (leftType != DataType_String)
            {
                result.result = AspRunResult_UnexpectedType;
                break;
            }

            /* TODO: Implement. */
            result.result = AspRunResult_NotImplemented;
            break;
        }

        case DataType_Tuple:
        case DataType_List:
        {
            for (AspSequenceResult nextResult = AspSequenceNext
                    (engine, right, 0);
                 !isIn && nextResult.element != 0;
                 nextResult = AspSequenceNext
                    (engine, right, nextResult.element))
            {
                AspDataEntry *value = nextResult.value;
                isIn = IsEqual(engine, left, value);
            }
            break;
        }

        case DataType_Set:
        case DataType_Dictionary:
        {
            AspTreeResult findResult = AspTreeFind(engine, right, left);
            if (findResult.result != AspRunResult_OK)
            {
                result.result = findResult.result;
                break;
            }
            isIn = findResult.node != 0;
            break;
        }
    }

    if (result.result != AspRunResult_OK)
        return result;

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
        result.value = AspNewBoolean(engine, resultValue);
        if (result.value == 0)
            result.result = AspRunResult_OutOfDataMemory;
    }

    return result;
}

static AspOperationResult PerformIdentityOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    bool isIdentical = left == right;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_NIS:
            resultValue = !isIdentical;
            break;

        case OpCode_IS:
            resultValue = isIdentical;
            break;
    }

    if (result.result == AspRunResult_OK)
    {
        result.value = AspNewBoolean(engine, resultValue);
        if (result.value == 0)
            result.result = AspRunResult_OutOfDataMemory;
    }

    return result;
}

static bool IsEqual
    (AspEngine *engine, AspDataEntry *left, AspDataEntry *right)
{
    if (left == right)
        return true;

    uint8_t leftType = AspDataGetType(left);
    uint8_t rightType = AspDataGetType(right);
    if (leftType != rightType)
        return false;

    switch (leftType)
    {
        default:
            break;

        case DataType_None:
        case DataType_Ellipsis:
            return true;

        case DataType_Boolean:
            return
                AspDataGetBoolean(left) == AspDataGetBoolean(right);

        case DataType_Integer:
            return
                AspDataGetInteger(left) == AspDataGetInteger(right);

        case DataType_Float:
            return
                AspDataGetFloat(left) == AspDataGetFloat(right);

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
            return
                leftStartValue == rightStartValue &&
                leftEndValue == rightEndValue &&
                leftStepValue == rightStepValue;
        }

        case DataType_String:
        {
            uint32_t leftCount = AspDataGetSequenceCount(left);
            uint32_t rightCount = AspDataGetSequenceCount(right);
            if (leftCount != rightCount)
                return false;
            for (uint32_t i = 0; i < leftCount; i++)
            {
                if (AspStringElement(engine, left, i) !=
                    AspStringElement(engine, right, i))
                    return false;
            }
            return true;
        }

        case DataType_Tuple:
        case DataType_List:
        case DataType_Set:
        case DataType_Dictionary:
            /* TODO: Handle collections "recursively" (but implement
               with iteration). */
            AspAssert(engine, false);
            return false;

        case DataType_Iterator:
        {
            uint32_t leftIterableIndex =
                AspDataGetIteratorIterableIndex(left);
            uint32_t rightIterableIndex =
                AspDataGetIteratorIterableIndex(right);
            uint32_t leftMemberIndex =
                AspDataGetIteratorMemberIndex(left);
            uint32_t rightMemberIndex =
                AspDataGetIteratorMemberIndex(right);
            if (leftIterableIndex != rightIterableIndex ||
                leftMemberIndex != rightMemberIndex)
                return false;
            AspDataEntry *iterable = AspValueEntry
                (engine, leftIterableIndex);
            return
                iterable == 0 ||
                AspDataGetType(iterable) != DataType_String ||
                AspDataGetIteratorStringIndex(left) ==
                AspDataGetIteratorStringIndex(right);
        }

        case DataType_Type:
            return
                AspDataGetTypeValue(left) == AspDataGetTypeValue(right);
    }
}
