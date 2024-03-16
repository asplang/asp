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

typedef struct
{
    AspRunResult result;
    int32_t intValue;
    AspDataEntry *value;
} AspRangeResult;

/* Normal range routines. */
void AspGetRange
    (AspEngine *engine, const AspDataEntry *range,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue);
AspRunResult AspRangeCount
    (AspEngine *, const AspDataEntry *range, int32_t *count);
AspRangeResult AspRangeIndex
    (AspEngine *, AspDataEntry *range, int32_t index, bool createObject);
bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue);

/* Slice routines. */
AspRunResult AspGetSliceRange
    (AspEngine *engine, const AspDataEntry *range, int32_t sequenceCount,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue);
AspRangeResult AspRangeSlice
    (AspEngine *, AspDataEntry *range, AspDataEntry *sliceRange);

#ifdef __cplusplus
}
#endif

#endif
