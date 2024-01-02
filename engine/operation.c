/*
 * Asp engine operations implementation.
 */

#include "operation.h"
#include "asp-priv.h"
#include "opcode.h"
#include "data.h"
#include "range.h"
#include "sequence.h"
#include "compare.h"
#include "tree.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

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
static AspOperationResult PerformRelationalOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformMembershipOperation
    (AspEngine *, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);
static AspOperationResult PerformIdentityOperation
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
                        (engine, opCode, left, right);
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
            result = PerformRelationalOperation
                (engine, opCode, left, right);
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
            int32_t repeatCountValue = AspDataGetInteger(repeatCount);
            if (repeatCountValue < 0)
            {
                result.result = AspRunResult_ValueOutOfRange;
                break;
            }

            /* Reuse the sequence argument as the return value if possible. */
            if ((AspCount(sequence) == 0 || repeatCountValue == 1) &&
                (sequenceType == DataType_String ||
                 sequenceType == DataType_Tuple))
            {
                AspRef(engine, sequence);
                result.value = sequence;
                break;
            }

            result.value = AspAllocEntry(engine, sequenceType);
            if (result.value == 0)
                break;

            AspSequenceResult appendResult = {AspRunResult_OK, 0, 0};
            if (AspCount(sequence) != 0)
            {
                for (int32_t i = 0; i < repeatCountValue; i++)
                {
                    for (AspSequenceResult nextResult = AspSequenceNext
                            (engine, sequence, 0);
                         nextResult.element != 0;
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
                            break;
                    }

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
     AspDataEntry *format, AspDataEntry *tuple)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    result.value = AspAllocEntry(engine, DataType_String);
    if (result.value == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }

    /* Scan format string and convert fields. */
    AspSequenceResult nextValueResult = {AspRunResult_OK, 0, 0};
    char formatBuffer[31], *fp = 0, formattedValueBuffer[61];
    for (AspSequenceResult nextResult = AspSequenceNext(engine, format, 0);
         nextResult.element != 0;
         nextResult = AspSequenceNext(engine, format, nextResult.element))
    {
        AspDataEntry *fragment = nextResult.value;
        uint8_t fragmentSize =
            AspDataGetStringFragmentSize(fragment);
        char *fragmentData =
            AspDataGetStringFragmentData(fragment);

        for (uint8_t fragmentIndex = 0;
             fragmentIndex < fragmentSize;
             fragmentIndex++)
        {
            char c = fragmentData[fragmentIndex];
            if (fp == 0)
            {
                /* Process non-format character. */
                if (c == '%')
                {
                    /* Switch to processing format characters. */
                    fp = formatBuffer;
                    *fp++ = '%';
                }
                else
                {
                    /* Copy the non-format character to the result. */
                    AspRunResult appendResult = AspStringAppendBuffer
                        (engine, result.value, &c, 1);
                    if (appendResult != AspRunResult_OK)
                    {
                        result.result = appendResult;
                        return result;
                    }
                }
            }
            else
            {
                /* Process format character. */
                if (c == '\0')
                {
                    result.result = AspRunResult_InvalidFormatString;
                    return result;
                }
                if (c == '%')
                {
                    if (fp != formatBuffer + 1)
                    {
                        result.result = AspRunResult_InvalidFormatString;
                        return result;
                    }

                    /* Copy the percent sign character to the result. */
                    AspRunResult appendResult = AspStringAppendBuffer
                        (engine, result.value, &c, 1);
                    if (appendResult != AspRunResult_OK)
                    {
                        result.result = appendResult;
                        return result;
                    }
                }
                else
                {
                    /* Continue to build format string for next value. */
                    if (fp >= formatBuffer + sizeof formatBuffer - 1)
                    {
                        result.result = AspRunResult_InvalidFormatString;
                        return result;
                    }

                    /* Ignore length specifiers */
                    if (c == 'L' || c == 'l' || c == 'h')
                        continue;

                    *fp++ = c;

                    /* Check for a conversion type character, which ends the
                       format string. */
                    bool
                        isInteger = false, isFloat = false,
                        isCharacter = false, isString = false;
                    if (strchr("diouxX", c) != 0)
                        isInteger = true;
                    else if (strchr("eEfFgG", c) != 0)
                        isFloat = true;
                    else if (c == 'c')
                        isCharacter = true;
                    else if (strchr("rsa", c) != 0)
                        isString = true;
                    else if ((c >= '0' && c <= '9') || c == '-' || c == '+'
                             || c == '.' || c == ' ' || c == '#')
                        continue;
                    else
                    {
                        /* Disallow unsupported and potentially dangerous
                           format specifiers */
                        result.result = AspRunResult_InvalidFormatString;
                        return result;
                    }

                    /* Prepare to format the next value. */
                    *fp = '\0';

                    /* Fetch the next value from the tuple. */
                    nextValueResult = AspSequenceNext
                        (engine, tuple, nextValueResult.element);
                    if (nextValueResult.result != AspRunResult_OK)
                    {
                        result.result = nextValueResult.result;
                        return result;
                    }
                    if (nextValueResult.element == 0)
                    {
                        result.result = AspRunResult_StringFormattingError;
                        return result;
                    }

                    /* Convert the next value to an appropriate type and
                       format it. */
                    AspDataEntry *nextValue = nextValueResult.value;
                    if (isString)
                    {
                        /* Validate the format string. */
                        int resultSize = snprintf(0, 0, formatBuffer, "");
                        if (resultSize < 0)
                        {
                            result.result = AspRunResult_StringFormattingError;
                            return result;
                        }

                        /* Extract width and precision fields from the format
                           string. */
                        bool leftJustify = strchr(formatBuffer, '-') != 0;
                        unsigned long width = 0, precision = ULONG_MAX;
                        const char *p = strpbrk(formatBuffer, "0123456789");
                        if (p != 0)
                        {
                            char *endp;
                            unsigned long n = strtoul(p, &endp, 10);
                            if (p != formatBuffer && *(p - 1) == '.')
                                precision = n;
                            else
                            {
                                width = n;
                                if (*endp == '.')
                                    precision = strtoul(endp + 1, 0, 10);
                            }
                        }

                        AspDataEntry *str = c == 's' ?
                            AspToString(engine, nextValue) :
                            AspToRepr(engine, nextValue);
                        if (str == 0)
                        {
                            result.result = AspRunResult_OutOfDataMemory;
                            return result;
                        }

                        /* Determine number of bytes of the string to print. */
                        unsigned long sourceSize = AspCount(str);
                        if (precision < sourceSize)
                            sourceSize = precision;

                        /* Determine the number of bytes to print. */
                        unsigned fieldSize = sourceSize;
                        if (width > fieldSize)
                            fieldSize = width;

                        /* Determine amount of padding. */
                        unsigned long paddingSize = 0;
                        if (fieldSize > sourceSize)
                            paddingSize = fieldSize - sourceSize;

                        /* Add left padding if applicable. */
                        static char space = ' ';
                        if (!leftJustify)
                        {
                            for (unsigned long i = 0; i < paddingSize; i++)
                            {
                                result.result = AspStringAppendBuffer
                                    (engine, result.value, &space, 1);
                                if (result.result != AspRunResult_OK)
                                    return result;
                            }
                        }

                        /* Append the applicable portion of the string value
                           to the result. */
                        unsigned long remainingSize = sourceSize;
                        for (AspSequenceResult strNextResult = AspSequenceNext
                                (engine, str, 0);
                             remainingSize > 0 && strNextResult.element != 0;
                             strNextResult = AspSequenceNext
                                (engine, str, strNextResult.element))
                        {
                            AspDataEntry *fragment = strNextResult.value;
                            uint8_t fragmentSize =
                                AspDataGetStringFragmentSize(fragment);
                            char *fragmentData =
                                AspDataGetStringFragmentData(fragment);

                            if (fragmentSize > remainingSize)
                                fragmentSize = (uint8_t)remainingSize;

                            result.result = AspStringAppendBuffer
                                (engine, result.value,
                                 fragmentData, fragmentSize);
                            if (result.result != AspRunResult_OK)
                                return result;

                            remainingSize -= fragmentSize;
                        }

                        /* Add right padding if applicable. */
                        if (leftJustify)
                        {
                            for (unsigned long i = 0; i < paddingSize; i++)
                            {
                                result.result = AspStringAppendBuffer
                                    (engine, result.value, &space, 1);
                                if (result.result != AspRunResult_OK)
                                    return result;
                            }
                        }

                        AspUnref(engine, str);
                        if (engine->runResult != AspRunResult_OK)
                        {
                            result.result = engine->runResult;
                            return result;
                        }
                    }
                    else
                    {
                        /* Format the non-string value. */
                        int formattedSize = 0;
                        if (isFloat)
                        {
                            double value;
                            if (!AspFloatValue(nextValue, &value))
                            {
                                result.result =
                                    AspRunResult_StringFormattingError;
                                return result;
                            }

                            formattedSize = snprintf
                                (formattedValueBuffer,
                                 sizeof formattedValueBuffer,
                                 formatBuffer, value);
                            if (formattedSize < 0 ||
                                formattedSize >= sizeof formattedValueBuffer)
                            {
                                result.result =
                                    AspRunResult_StringFormattingError;
                                return result;
                            }
                        }
                        else
                        {
                            int value;
                            if (isInteger)
                            {
                                int32_t i32;
                                if (!AspIntegerValue(nextValue, &i32))
                                {
                                    result.result =
                                        AspRunResult_StringFormattingError;
                                    return result;
                                }
                                value = (int)i32;
                            }
                            else if (isCharacter)
                            {
                                if (AspIsInteger(nextValue))
                                {
                                    int32_t i32;
                                    AspIntegerValue(nextValue, &i32);
                                    if (i32 < 0 || i32 > 0xFF)
                                    {
                                        result.result =
                                            AspRunResult_ValueOutOfRange;
                                        return result;
                                    }
                                    value = (int)i32;
                                }
                                else if (AspIsString(nextValue))
                                {
                                    if (AspCount(nextValue) != 1)
                                    {
                                        result.result =
                                            AspRunResult_ValueOutOfRange;
                                        return result;
                                    }
                                    char c;
                                    AspStringValue
                                        (engine, nextValue, 0, &c, 0, 1);
                                    value = c;
                                }
                                else
                                {
                                    result.result =
                                        AspRunResult_StringFormattingError;
                                    return result;
                                }
                            }
                            else
                            {
                                result.result = AspRunResult_InternalError;
                                return result;
                            }

                            formattedSize = snprintf
                                (formattedValueBuffer,
                                 sizeof formattedValueBuffer,
                                 formatBuffer, value);
                            if (formattedSize < 0 ||
                                formattedSize >= sizeof formattedValueBuffer)
                            {
                                result.result =
                                    AspRunResult_StringFormattingError;
                                return result;
                            }
                        }

                        /* Append the formatted value to the result. */
                        result.result = AspStringAppendBuffer
                            (engine, result.value,
                             formattedValueBuffer, formattedSize);
                        if (result.result != AspRunResult_OK)
                            return result;
                    }
                }

                /* Switch back to processing non-format characters. */
                fp = 0;
            }
        }
    }

    /* Ensure the format was well formed. */
    if (fp != 0)
    {
        result.result = AspRunResult_InvalidFormatString;
        return result;
    }

    /* Ensure all the values in the tuple were used. */
    nextValueResult = AspSequenceNext
        (engine, tuple, nextValueResult.element);
    if (nextValueResult.result != AspRunResult_OK)
    {
        result.result = nextValueResult.result;
        return result;
    }
    if (nextValueResult.element != 0)
    {
        result.result = AspRunResult_StringFormattingError;
        return result;
    }

    return result;
}

static AspOperationResult PerformEqualityOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    int comparison = 0;
    result.result = AspCompare
        (engine, left, right, AspCompareType_Equality,
         &comparison, 0);
    if (result.result != AspRunResult_OK)
        return result;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_NE:
            resultValue = comparison != 0;
            break;

        case OpCode_EQ:
            resultValue = comparison == 0;
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

static AspOperationResult PerformRelationalOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right)
{
    AspOperationResult result = {AspRunResult_OK, 0};

    int comparison = 0;
    bool nanDetected = false;
    result.result = AspCompare
        (engine, left, right, AspCompareType_Relational,
         &comparison, &nanDetected);
    if (result.result != AspRunResult_OK)
        return result;

    bool resultValue = false;
    switch (opCode)
    {
        default:
            result.result = AspRunResult_InvalidInstruction;
            break;

        case OpCode_LT:
            resultValue = !nanDetected && comparison < 0;
            break;

        case OpCode_LE:
            resultValue = !nanDetected && comparison <= 0;
            break;

        case OpCode_GT:
            resultValue = !nanDetected && comparison > 0;
            break;

        case OpCode_GE:
            resultValue = !nanDetected && comparison >= 0;
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

            /* Search for substring. */
            int
                leftCount = (int)AspDataGetSequenceCount(left),
                rightCount = (int)AspDataGetSequenceCount(right);
            if (leftCount == 0)
                isIn = true;
            else if (leftCount > rightCount)
                isIn = false;
            else
            {
                for (int index = 0;
                     index <= rightCount - leftCount;
                     index++)
                {
                    isIn = true;
                    int rightIndex = index;
                    for (int leftIndex = 0; leftIndex < leftCount;
                         leftIndex++, rightIndex++)
                    {
                        if (AspStringElement(engine, left, leftIndex) !=
                            AspStringElement(engine, right, rightIndex))
                        {
                            isIn = false;
                            break;
                        }
                    }
                    if (isIn)
                        break;
                }
            }

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
                int comparison;
                result.result = AspCompare
                    (engine, left, value, AspCompareType_Equality,
                     &comparison, 0);
                if (result.result != AspRunResult_OK)
                    break;
                isIn = comparison == 0;
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
