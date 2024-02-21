/*
 * Asp engine integer arithmetic definitions.
 */

#ifndef ASP_INTEGER_H
#define ASP_INTEGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AspIntegerResult
{
    AspIntegerResult_OK,
    AspIntegerResult_ValueOutOfRange,
    AspIntegerResult_DivideByZero,
    AspIntegerResult_ArithmeticOverflow,
} AspIntegerResult;

AspIntegerResult AspNegateInteger(int32_t value, int32_t *result);
AspIntegerResult AspAddIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspSubtractIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspMultiplyIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspDivideIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspModuloIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspBitwiseOrIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspBitwiseExclusiveOrIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspBitwiseAndIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspLeftShiftInteger
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspRightShiftInteger
    (int32_t left, int32_t right, int32_t *result);

#ifdef __cplusplus
}
#endif

#endif
