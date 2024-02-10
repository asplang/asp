/*
 * Asp engine arithmetic definitions.
 */

#ifndef ASP_ARITHMETIC_H
#define ASP_ARITHMETIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"

AspRunResult AspAddIntegers(int32_t left, int32_t right, int32_t *result);

#ifdef __cplusplus
}
#endif

#endif
