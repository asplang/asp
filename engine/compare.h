/*
 * Asp engine object comparison definitions.
 */

#ifndef ASP_COMPARE_H
#define ASP_COMPARE_H

#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    AspCompareType_Equality,
    AspCompareType_Relational,
    AspCompareType_Key,
} AspCompareType;

AspRunResult AspCompare
    (AspEngine *engine,
     const AspDataEntry *left, const AspDataEntry *right,
     AspCompareType, int *result, bool *nanDetected);

#ifdef __cplusplus
}
#endif

#endif
