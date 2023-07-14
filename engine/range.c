/*
 * Asp engine range implementation.
 */

#include "range.h"
#include "asp.h"
#include "data.h"
#include <limits.h>

void AspGetRange
    (AspEngine *engine, const AspDataEntry *entry,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue)
{
    AspAssert
        (engine, entry != 0 && AspDataGetType(entry) == DataType_Range);

    if (startValue != 0)
    {
        AspDataEntry *start = 0;
        if (AspDataGetRangeHasStart(entry))
        {
            start = AspValueEntry
                (engine, AspDataGetRangeStartIndex(entry));
            AspAssert(engine, AspDataGetType(start) == DataType_Integer);
        }
        *startValue =
            start == 0 ? 0 : AspDataGetInteger(start);
    }

    int32_t localStepValue;
    if (endValue != 0 || stepValue != 0)
    {
        AspDataEntry *step = 0;
        if (AspDataGetRangeHasStep(entry))
        {
            step = AspValueEntry
                (engine, AspDataGetRangeStepIndex(entry));
            AspAssert(engine, AspDataGetType(step) == DataType_Integer);
        }
        localStepValue = step == 0 ? 1 : AspDataGetInteger(step);
        if (stepValue != 0)
            *stepValue = localStepValue;
    }

    if (endValue != 0)
    {
        AspDataEntry *end = 0;
        if (AspDataGetRangeHasEnd(entry))
        {
            end = AspValueEntry
                (engine, AspDataGetRangeEndIndex(entry));
            AspAssert(engine, AspDataGetType(end) == DataType_Integer);
        }
        *endValue =
            end == 0 ? localStepValue < 0 ? INT32_MIN : INT32_MAX :
            AspDataGetInteger(end);
    }
}

/* Prepares for slice operations by limiting the components to valid indices
   for the given sequence size and making all components agree in sign. */
void AspGetSliceRange
    (AspEngine *engine, const AspDataEntry *entry, int32_t sequenceCount,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue)
{
    AspGetRange
        (engine, entry,
         startValue, endValue, stepValue);

    /* Ensure the start and end are within range. */
    if (*startValue < 0)
    {
        if (*startValue < -sequenceCount)
            *startValue = -1 - sequenceCount;
    }
    else
    {
        if (*startValue >= sequenceCount)
            *startValue = sequenceCount;
    }
    if (*endValue < 0)
    {
        if (*endValue < -1 - sequenceCount)
            *endValue = -1 - sequenceCount;
    }
    else
    {
        if (*endValue > sequenceCount)
            *endValue = sequenceCount;
    }

    /* Adjust the start and end values to agree with the sign of the step. */
    if (*startValue < 0 != *stepValue < 0)
    {
        *startValue =
            *startValue < -sequenceCount ? *startValue = 0 :
            *startValue >= sequenceCount ? *startValue = -1 :
            *startValue < 0 ?
                *startValue + sequenceCount : *startValue - sequenceCount;
    }
    if (*endValue < 0 != *stepValue < 0)
    {
        *endValue =
            *endValue < -sequenceCount ? *endValue = 0 :
            *endValue >= sequenceCount ? *endValue = -1 :
            *endValue < 0 ?
                *endValue + sequenceCount : *endValue - sequenceCount;
    }
}

bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue)
{
    return
        stepValue == 0 ? testValue == endValue : stepValue > 0 ?
        endValue != INT32_MAX && testValue >= endValue :
        endValue != INT32_MIN && testValue <= endValue;
}
