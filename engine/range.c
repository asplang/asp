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

bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue)
{
    return
        stepValue == 0 ? testValue == endValue : stepValue > 0 ?
        endValue != INT32_MAX && testValue >= endValue :
        endValue != INT32_MIN && testValue <= endValue;
}
