/*
 * Asp engine integer arithmetic implementation.
 */

#include "integer.h"
#include <stdbool.h>

AspIntegerResult AspNegateInteger(int32_t value, int32_t *result)
{
    if (value == INT32_MIN)
        return AspIntegerResult_ArithmeticOverflow;

    *result = -value;

    return AspIntegerResult_OK;
}

AspIntegerResult AspAddIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    /* Perform the operation on unsigned representations to avoid overflow. */
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft + uRight;
    *result = *(int32_t *)&uResult;

    /* Check for overflow conditions. */
    return
        *result < left != right < 0 ?
        AspIntegerResult_ArithmeticOverflow : AspIntegerResult_OK;
}

AspIntegerResult AspSubtractIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    /* Check for overflow conditions before performing the operation. */
    bool overflow =
        right > 0 && left < INT32_MIN + right ||
        right < 0 && left > INT32_MAX + right;
    if (!overflow)
        *result = left - right;

    return
        overflow ?
        AspIntegerResult_ArithmeticOverflow : AspIntegerResult_OK;
}

AspIntegerResult AspMultiplyIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    /* Perform the operation on unsigned representations to avoid overflow. */
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft * uRight;
    *result = *(int32_t *)&uResult;

    /* Check for overflow conditions. */
    return
        left == INT32_MIN && right == -1 ||
        left != 0 && right != 0 && left != *result / right ?
        AspIntegerResult_ArithmeticOverflow : AspIntegerResult_OK;
}

AspIntegerResult AspDivideIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    if (right == 0)
        return AspIntegerResult_DivideByZero;

    /* Check for overflow conditions before performing the operation. */
    bool overflow = left == INT32_MIN && right == -1;
    if (!overflow)
    {
        /* Round down (like floor), as Python does, not towards zero, as C99
           specifies. */
        *result = left / right;
        if (left < 0 != right < 0 && left % right != 0)
            (*result)--;
    }

    return
        overflow ?
        AspIntegerResult_ArithmeticOverflow : AspIntegerResult_OK;
}

AspIntegerResult AspModuloIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    if (right == 0)
        return AspIntegerResult_DivideByZero;

    /* Perform modulo operation as Python does while avoiding undefined
       behaviours due to overflow conditions. */
    *result = right == -1 || left == right ? 0 : left % right;
    if (left < 0 != right < 0 && *result != 0)
        *result += right;

    return AspIntegerResult_OK;
}

AspIntegerResult AspBitwiseOrIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft | uRight;
    *result = *(int32_t *)&uResult;

    return AspIntegerResult_OK;
}

AspIntegerResult AspBitwiseExclusiveOrIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft ^ uRight;
    *result = *(int32_t *)&uResult;

    return AspIntegerResult_OK;
}

AspIntegerResult AspBitwiseAndIntegers
    (int32_t left, int32_t right, int32_t *result)
{
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft & uRight;
    *result = *(int32_t *)&uResult;

    return AspIntegerResult_OK;
}

AspIntegerResult AspLeftShiftInteger
    (int32_t left, int32_t right, int32_t *result)
{
   if (right < 0)
        return AspIntegerResult_ValueOutOfRange;

    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = right >= 32 ? 0 : uLeft << uRight;
    *result = *(int32_t *)&uResult;

    return AspIntegerResult_OK;
}

AspIntegerResult AspRightShiftInteger
    (int32_t left, int32_t right, int32_t *result)
{
    if (right < 0)
        return AspIntegerResult_ValueOutOfRange;

    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult;
    if (right >= 32)
        uResult = left < 0 ? -1 : 0;
    else
    {
        /* Perform sign extension. */
        uResult = uLeft >> uRight;
        if (left < 0 && uRight != 0)
            uResult |= (1U << uRight) - 1U << 32U - uRight;
    }
    *result = *(int32_t *)&uResult;

    return AspIntegerResult_OK;
}
