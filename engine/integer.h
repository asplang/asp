/*
 * Asp engine integer arithmetic definitions.
 */

#ifndef ASP_INTEGER_H
#define ASP_INTEGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "integer-result.h"
#include <stdint.h>

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
AspIntegerResult AspLeftShiftIntegers
    (int32_t left, int32_t right, int32_t *result);
AspIntegerResult AspRightShiftIntegers
    (int32_t left, int32_t right, int32_t *result);

#ifdef __cplusplus
}
#endif

#endif
