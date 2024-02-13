/*
 * Asp engine integer arithmetic definitions.
 */

#ifndef ASP_INTEGER_H
#define ASP_INTEGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"
#include <stdint.h>

AspRunResult AspNegateInteger(int32_t value, int32_t *result);
AspRunResult AspAddIntegers(int32_t left, int32_t right, int32_t *result);
AspRunResult AspSubtractIntegers(int32_t left, int32_t right, int32_t *result);
AspRunResult AspMultiplyIntegers(int32_t left, int32_t right, int32_t *result);
AspRunResult AspDivideIntegers(int32_t left, int32_t right, int32_t *result);
AspRunResult AspModuloIntegers(int32_t left, int32_t right, int32_t *result);

#ifdef __cplusplus
}
#endif

#endif
