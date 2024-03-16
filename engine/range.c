/*
 * Asp engine range implementation.
 */

#include "range.h"
#include "integer.h"
#include "integer-result.h"
#include "asp.h"
#include "data.h"
#include <stdint.h>

static AspRunResult LimitIndex
    (int32_t *index, int32_t step, int32_t count);
static AspRunResult UnboundedRangeIndex
    (int32_t start, int32_t step, int32_t count, int32_t index,
     int32_t *rangeIndex);

void AspGetRange
    (AspEngine *engine, const AspDataEntry *range,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue)
{
    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    if (engine->runResult != AspRunResult_OK)
        return;

    if (startValue != 0)
    {
        AspDataEntry *start = 0;
        if (AspDataGetRangeHasStart(range))
        {
            start = AspValueEntry
                (engine, AspDataGetRangeStartIndex(range));
            AspAssert(engine, AspDataGetType(start) == DataType_Integer);
        }
        *startValue = start == 0 ? 0 : AspDataGetInteger(start);
    }

    int32_t localStepValue;
    if (endValue != 0 || stepValue != 0)
    {
        AspDataEntry *step = 0;
        if (AspDataGetRangeHasStep(range))
        {
            step = AspValueEntry
                (engine, AspDataGetRangeStepIndex(range));
            AspAssert(engine, AspDataGetType(step) == DataType_Integer);
        }
        localStepValue = step == 0 ? 1 : AspDataGetInteger(step);
        if (stepValue != 0)
            *stepValue = localStepValue;
    }

    if (endValue != 0)
    {
        AspDataEntry *end = 0;
        if (AspDataGetRangeHasEnd(range))
        {
            end = AspValueEntry
                (engine, AspDataGetRangeEndIndex(range));
            AspAssert(engine, AspDataGetType(end) == DataType_Integer);
        }
        *endValue =
            end == 0 ? localStepValue < 0 ? INT32_MIN : INT32_MAX :
            AspDataGetInteger(end);
    }
}

AspRunResult AspRangeCount
    (AspEngine *engine, const AspDataEntry *range, int32_t *count)
{
    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    if (engine->runResult != AspRunResult_OK)
        return 0;

    int32_t start, end, step;
    AspGetRange(engine, range, &start, &end, &step);

    /* Deal with infinite ranges. */
    if (step == 0)
        return AspRunResult_ValueOutOfRange;

    /* Check for an empty range. */
    if (step < 0 ? end >= start : start >= end)
    {
        *count = 0;
        return AspRunResult_OK;
    }

    /* Deal with negative step. */
    if (step < 0)
    {
        /* Swap the start and end values. */
        start ^= end;
        end ^= start;
        start ^= end;

        /* Negate the step. */
        AspIntegerResult integerResult = AspNegateInteger(step, &step);
        if (integerResult != AspIntegerResult_OK)
            return AspTranslateIntegerResult(integerResult);
    }

    /* Compute the count as ((end - start - 1) // step + 1). */
    int32_t resultValue;
    AspIntegerResult integerResult = AspSubtractIntegers
        (end, start, &resultValue);
    if (integerResult == AspIntegerResult_OK)
        integerResult = AspSubtractIntegers(resultValue, 1, &resultValue);
    if (integerResult == AspIntegerResult_OK)
        integerResult = AspDivideIntegers(resultValue, step, &resultValue);
    if (integerResult == AspIntegerResult_OK)
        integerResult = AspAddIntegers(resultValue, 1, &resultValue);
    if (integerResult != AspIntegerResult_OK)
        return AspTranslateIntegerResult(integerResult);

    *count = resultValue;
    return AspRunResult_OK;
}

AspRangeResult AspRangeIndex
    (AspEngine *engine, const AspDataEntry *range, int32_t index,
     bool createObject)
{
    AspRangeResult result = {AspRunResult_OK, 0, 0};

    result.result = AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    if (result.result != AspRunResult_OK)
        return result;

    int32_t count;
    result.result = AspRangeCount(engine, range, &count);
    if (result.result != AspRunResult_OK)
        return result;

    int32_t start, end, step;
    AspGetRange(engine, range, &start, &end, &step);

    /* Ensure the index is in range. Note that since count is always
       non-negative, (-count) will always be representable. */
    if (index < -count || index >= count)
    {
        result.result = AspRunResult_ValueOutOfRange;
        return result;
    }

    int32_t rangeIndexResult;
    result.result = UnboundedRangeIndex
        (start, step, count, index, &rangeIndexResult);
    if (result.result != AspRunResult_OK)
        return result;

    /* Create an integer object if requested. */
    if (createObject)
    {
        result.value = AspNewInteger(engine, rangeIndexResult);
        if (result.value == 0)
        {
            result.result = AspRunResult_OutOfDataMemory;
            return result;
        }
    }

    return result;
}

bool AspIsValueAtRangeEnd
    (int32_t testValue, int32_t endValue, int32_t stepValue)
{
    return
        stepValue == 0 ? testValue == endValue : stepValue > 0 ?
        endValue != INT32_MAX && testValue >= endValue :
        endValue != INT32_MIN && testValue <= endValue;
}

/* Prepares for slice operations by limiting the components to valid indices
   for the given sequence size and making all components agree in sign.
   Note: This routine is intended for use with normal sequences, not ranges.
   For range slicing, use AspRangeSlice, which deals with large numbers. */
AspRunResult AspGetSliceRange
    (AspEngine *engine, const AspDataEntry *range, int32_t sequenceCount,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    result = AspAssert
        (engine, sequenceCount <= AspSignedWordMax);
    if (result != AspRunResult_OK)
        return result;

    AspGetRange(engine, range, startValue, endValue, stepValue);

    /* Adjust the start and end values for the slice operation. */
    int32_t *indexValues[] = {startValue, endValue};
    for (unsigned i = 0; i < sizeof indexValues / sizeof *indexValues; i++)
    {
        /* Limit the slice index with respect to the number of sequence
           elements. */
        result = LimitIndex(startValue, *stepValue, sequenceCount);
        if (result != AspRunResult_OK)
            return result;

        /* Adjust the index value to agree with the sign of the step value. */
        int32_t *indexValue = indexValues[i];
        if (*indexValue < 0 != *stepValue < 0)
        {
            *indexValue =
                *indexValue < -sequenceCount ? 0 :
                *indexValue >= sequenceCount ? -1 :
                *indexValue < 0 ?
                    *indexValue + sequenceCount : *indexValue - sequenceCount;
        }
    }

    return AspRunResult_OK;
}

AspRangeResult AspRangeSlice
    (AspEngine *engine, const AspDataEntry *range, AspDataEntry *sliceRange)
{
    AspRangeResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine,
         range != 0 && AspDataGetType(range) == DataType_Range);
    result.result = AspAssert
        (engine,
         sliceRange != 0 && AspDataGetType(sliceRange) == DataType_Range);
    if (result.result != AspRunResult_OK)
        return result;

    int32_t start, end, step;
    AspGetRange(engine, range, &start, &end, &step);
    int32_t count;
    result.result = AspRangeCount(engine, range, &count);
    if (result.result != AspRunResult_OK)
        return result;
    int32_t sliceStart, sliceEnd, sliceStep;
    AspGetRange(engine, sliceRange, &sliceStart, &sliceEnd, &sliceStep);

    /* Limit the slice indices with respect to the number of range elements. */
    result.result = LimitIndex(&sliceStart, sliceStep, count);
    if (result.result != AspRunResult_OK)
        return result;
    result.result = LimitIndex(&sliceEnd, sliceStep, count);
    if (result.result != AspRunResult_OK)
        return result;

    /* Compute the new range as
       (range[sliceStart], range[sliceEnd], step * sliceStep). */
    int32_t newStart, newEnd, newStep;
    result.result = UnboundedRangeIndex
        (start, step, count, sliceStart, &newStart);
    if (result.result != AspRunResult_OK)
        return result;
    result.result = UnboundedRangeIndex
        (start, step, count, sliceEnd, &newEnd);
    if (result.result != AspRunResult_OK)
        return result;
    AspIntegerResult integerResult =
        AspMultiplyIntegers(step, sliceStep, &newStep);
    if (integerResult != AspIntegerResult_OK)
    {
        result.result = AspTranslateIntegerResult(integerResult);
        return result;
    }

    /* Create a new range object. */
    result.value = AspNewRange(engine, newStart, newEnd, newStep);
    if (result.value == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }

    return result;
}

static AspRunResult LimitIndex(int32_t *index, int32_t step, int32_t count)
{
    /* Note that since count is always non-negative, all computed values are
       representable, including (-count - 1) = (-1 - count). */
    if (*index < -count)
        *index = step < 0 ? -1 - count : 0;
    else if (*index >= count)
        *index = step < 0 ? -1 : count;

    return AspRunResult_OK;
}

static AspRunResult UnboundedRangeIndex
    (int32_t start, int32_t step, int32_t count, int32_t index,
     int32_t *rangeIndex)
{
    AspRangeResult result = {AspRunResult_OK, 0, 0};

    /* Deal with negative index. Note that since count is always non-negative,
       adding any negative value to it will be representable. */
    if (index < 0)
        index += count;

    /* Compute the value as (start + index * step). */
    int32_t resultValue;
    AspIntegerResult integerResult =
        AspMultiplyIntegers(index, step, &resultValue);
    if (integerResult == AspIntegerResult_OK)
        integerResult = AspAddIntegers
            (start, resultValue, &resultValue);
    if (integerResult != AspIntegerResult_OK)
        return AspTranslateIntegerResult(integerResult);

    *rangeIndex = resultValue;
    return AspRunResult_OK;
}
