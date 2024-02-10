/*
 * Asp engine arithmetic implementation.
 */

#include "arithmetic.h"
#include <stdint.h>

AspRunResult AspAddIntegers(int32_t left, int32_t right, int32_t *result)
{
    uint32_t uLeft = *(uint32_t *)&left, uRight = *(uint32_t *)&right;
    uint32_t uResult = uLeft + uRight;
    *result = *(int32_t *)&uResult;
    return
        *result < left != right < 0 ?
        AspRunResult_ArithmeticOverflow : AspRunResult_OK;
}
