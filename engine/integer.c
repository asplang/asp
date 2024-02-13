/*
 * Asp engine integer arithmetic implementation.
 */

#include "integer.h"

AspRunResult AspNegateInteger(int32_t value, int32_t *result)
{
    if (value == INT32_MIN)
        return AspRunResult_ArithmeticOverflow;

    *result = -value;

    return AspRunResult_OK;
}

AspRunResult AspAddIntegers(int32_t left, int32_t right, int32_t *result)
{
    /* Perform the operation on unsigned representations to avoid overflow. */
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft + uRight;
    *result = *(int32_t *)&uResult;

    /* Check for overflow conditions. */
    return
        *result < left != right < 0 ?
        AspRunResult_ArithmeticOverflow : AspRunResult_OK;
}

AspRunResult AspSubtractIntegers(int32_t left, int32_t right, int32_t *result)
{
    /* Check for overflow conditions before performing the operation. */
    bool overflow =
        right > 0 && left < INT32_MIN + right ||
        right < 0 && left > INT32_MAX + right;
    if (!overflow)
        *result = left - right;

    return
        overflow ?
        AspRunResult_ArithmeticOverflow : AspRunResult_OK;
}

AspRunResult AspMultiplyIntegers(int32_t left, int32_t right, int32_t *result)
{
    /* Perform the operation on unsigned representations to avoid overflow. */
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft * uRight;
    *result = *(int32_t *)&uResult;

    /* Check for overflow conditions. */
    return
        left == INT32_MIN && right == -1 ||
        left != 0 && right != 0 && left != *result / right ?
        AspRunResult_ArithmeticOverflow : AspRunResult_OK;
}

AspRunResult AspDivideIntegers(int32_t left, int32_t right, int32_t *result)
{
    if (right == 0)
        return AspRunResult_DivideByZero;

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
        AspRunResult_ArithmeticOverflow : AspRunResult_OK;
}

AspRunResult AspModuloIntegers(int32_t left, int32_t right, int32_t *result)
{
    if (right == 0)
        return AspRunResult_DivideByZero;

    /* Perform modulo operation as Python does while avoiding undefined
       behaviours due to overflow conditions. */
    *result = right == -1 || left == right ? 0 : left % right;
    if (left < 0 != right < 0 && *result != 0)
        *result += right;

    return AspRunResult_OK;
}
