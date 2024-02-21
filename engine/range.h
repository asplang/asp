/*
 * Asp engine range definitions.
 */

#ifndef ASP_RANGE_H
#define ASP_RANGE_H

#include "asp-priv.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AspGetRange
    (AspEngine *engine, const AspDataEntry *entry,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue);
void AspGetSliceRange
    (AspEngine *engine, const AspDataEntry *entry, int32_t sequenceCount,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue);
bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue);

#ifdef __cplusplus
}
#endif

#endif
