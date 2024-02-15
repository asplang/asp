/*
 * Asp engine integer arithmetic result definitions.
 */

#ifndef ASP_INTEGER_RESULT_H
#define ASP_INTEGER_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"
#include <stdint.h>

typedef enum AspIntegerResult
{
    AspIntegerResult_OK,
    AspIntegerResult_ValueOutOfRange,
    AspIntegerResult_DivideByZero,
    AspIntegerResult_ArithmeticOverflow,
} AspIntegerResult;

AspRunResult AspTranslateIntegerResult(AspIntegerResult);

#ifdef __cplusplus
}
#endif

#endif
