/*
 * Asp engine range definitions.
 */

#ifndef ASP_RANGE_H
#define ASP_RANGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"
#include <stdint.h>

void AspGetRange
    (AspEngine *engine, const AspDataEntry *entry,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue);
bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue);

#ifdef __cplusplus
}
#endif

#endif
