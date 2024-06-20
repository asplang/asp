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
     int32_t *startValue, int32_t *endValue, int32_t *stepValue,
     bool *bounded)
{
    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    if (engine->runResult != AspRunResult_OK)
        return;

    /* Extract values from the range object. */
    AspDataEntry *startEntry = 0, *endEntry = 0, *stepEntry = 0;
    if (AspDataGetRangeHasStart(range))
    {
        startEntry = AspValueEntry
            (engine, AspDataGetRangeStartIndex(range));
        AspAssert(engine, AspDataGetType(startEntry) == DataType_Integer);
    }
    if (AspDataGetRangeHasEnd(range))
    {
        endEntry = AspValueEntry
            (engine, AspDataGetRangeEndIndex(range));
        AspAssert(engine, AspDataGetType(endEntry) == DataType_Integer);
    }
    if (AspDataGetRangeHasStep(range))
    {
        stepEntry = AspValueEntry
            (engine, AspDataGetRangeStepIndex(range));
        AspAssert(engine, AspDataGetType(stepEntry) == DataType_Integer);
    }
    int32_t localStepValue =
        stepEntry != 0 ? AspDataGetInteger(stepEntry) : 1;
    int32_t localStartValue =
        startEntry != 0 ? AspDataGetInteger(startEntry) :
        localStepValue < 0 ? -1 : 0;
    bool localBounded = endEntry != 0;
    int32_t localEndValue =
        localBounded ? AspDataGetInteger(endEntry) :
        localStepValue < 0 ? INT32_MIN : INT32_MAX;

    /* Populate requested return values. */
    if (startValue != 0)
        *startValue = localStartValue;
    if (endValue != 0)
        *endValue = localEndValue;
    if (stepValue != 0)
        *stepValue = localStepValue;
    if (bounded != 0)
        *bounded = localBounded;
}

AspRunResult AspRangeCount
    (AspEngine *engine, const AspDataEntry *range, int32_t *count)
{
    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    if (engine->runResult != AspRunResult_OK)
        return 0;

    int32_t start, end, step;
    bool bounded;
    AspGetRange(engine, range, &start, &end, &step, &bounded);

    /* Deal with infinite ranges. */
    if (step == 0 || !bounded)
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
    bool bounded;
    AspGetRange(engine, range, &start, &end, &step, &bounded);

    /* Ensure the index is in range. Note that since count is always
       non-negative, (-count) will always be representable. */
    if (bounded && (index < -count || index >= count))
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
    (int32_t testValue, int32_t endValue, int32_t stepValue, bool bounded)
{
    return
        bounded &&
        (stepValue == 0 ? testValue == endValue :
         stepValue < 0 ? testValue <= endValue : testValue >= endValue);
}

/* Prepares for slice operations by limiting the components to valid indices
   for the given sequence size and making all components agree in sign.
   Note: This routine is intended for use with normal sequences, not ranges.
   For range slicing, use AspRangeSlice, which deals with large numbers. */
AspRunResult AspGetSliceRange
    (AspEngine *engine, const AspDataEntry *range, int32_t sequenceCount,
     int32_t *startValue, int32_t *endValue, int32_t *stepValue,
     bool *bounded)
{
    AspRunResult result = AspRunResult_OK;

    AspAssert
        (engine, range != 0 && AspDataGetType(range) == DataType_Range);
    result = AspAssert
        (engine, sequenceCount <= AspSignedWordMax);
    if (result != AspRunResult_OK)
        return result;

    AspGetRange(engine, range, startValue, endValue, stepValue, bounded);

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

    int32_t rangeStart, rangeEnd, rangeStep;
    bool rangeBounded;
    AspGetRange
        (engine, range, &rangeStart, &rangeEnd, &rangeStep, &rangeBounded);
    int32_t rangeCount = 0;
    int32_t sliceStart, sliceEnd, sliceStep;
    bool sliceBounded;
    AspGetRange
        (engine, sliceRange,
         &sliceStart, &sliceEnd, &sliceStep, &sliceBounded);

    if (rangeBounded || sliceBounded)
    {
        result.result = AspRangeCount
            (engine, rangeBounded ? range : sliceRange, &rangeCount);
        if (result.result != AspRunResult_OK)
            return result;
    }

    /* Limit the slice indices with respect to the number of range elements. */
    if (rangeBounded)
    {
        result.result = LimitIndex(&sliceStart, sliceStep, rangeCount);
        if (result.result != AspRunResult_OK)
            return result;
        result.result = LimitIndex(&sliceEnd, sliceStep, rangeCount);
        if (result.result != AspRunResult_OK)
            return result;
    }

    /* Compute the new range as
       (range[sliceStart], range[sliceEnd], rangeStep * sliceStep), ensuring
       that for unbounded ranges, the slice contains no negative indexing
       elements. */
    int32_t newStart, newEnd = 0, newStep;
    result.result = UnboundedRangeIndex
        (rangeStart, rangeStep, rangeCount, sliceStart, &newStart);
    if (result.result != AspRunResult_OK)
        return result;
    if (!rangeBounded)
    {
        bool sliceIndexIsNegative = sliceStart < 0;
        if (!sliceIndexIsNegative)
        {
            if (!sliceBounded)
                sliceIndexIsNegative = sliceStep < 0;
            else if (rangeCount > 1)
            {
                int32_t lastSliceIndex;
                result.result = UnboundedRangeIndex
                    (sliceStart, sliceStep, rangeCount, -1, &lastSliceIndex);
                if (result.result != AspRunResult_OK)
                    return result;
                sliceIndexIsNegative = lastSliceIndex < 0;
            }
        }
        if (sliceIndexIsNegative)
        {
            result.result = AspRunResult_ValueOutOfRange;
            return result;
        }
    }
    if (rangeBounded || sliceBounded)
    {
        result.result = UnboundedRangeIndex
            (rangeStart, rangeStep,
             rangeBounded ? rangeCount : 0, sliceEnd, &newEnd);
        if (result.result != AspRunResult_OK)
            return result;
    }
    AspIntegerResult integerResult =
        AspMultiplyIntegers(rangeStep, sliceStep, &newStep);
    if (integerResult != AspIntegerResult_OK)
    {
        result.result = AspTranslateIntegerResult(integerResult);
        return result;
    }

    /* Create a new range object. */
    result.value = rangeBounded || sliceBounded ?
        AspNewRange(engine, newStart, newEnd, newStep) :
        AspNewUnboundedRange(engine, newStart, newStep);
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
