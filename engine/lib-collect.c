/*
 * Asp script function library implementation: collection functions.
 */

#include "asp.h"
#include "data.h"
#include "sequence.h"
#include "iterator.h"

static AspRunResult FillSequence
    (AspEngine *, AspDataEntry *sequence, AspDataEntry *iterable);

/* tuple(x)
 * Convert the iterable to a tuple.
 */
ASP_LIB_API AspRunResult AspLib_tuple
    (AspEngine *engine,
     AspDataEntry *iterable,
     AspDataEntry **returnValue)
{
    if (AspIsTuple(iterable))
    {
        AspRef(engine, iterable);
        *returnValue = iterable;
        return AspRunResult_OK;
    }

    *returnValue = AspNewTuple(engine);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;

    return FillSequence(engine, *returnValue, iterable);
}

/* list(x)
 * Convert the iterable to a list.
 */
ASP_LIB_API AspRunResult AspLib_list
    (AspEngine *engine,
     AspDataEntry *iterable,
     AspDataEntry **returnValue)
{
    *returnValue = AspNewList(engine);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;

    return FillSequence(engine, *returnValue, iterable);
}

static AspRunResult FillSequence
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *iterable)
{
    uint8_t iterableType = AspDataGetType(iterable);
    if (iterableType == DataType_None)
        return AspRunResult_OK;

    AspIteratorResult iteratorResult = AspIteratorCreate
        (engine, iterable, false);
    if (iteratorResult.result != AspRunResult_OK)
        return iteratorResult.result;
    AspDataEntry *iterator = iteratorResult.value;

    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        iteratorResult = AspIteratorDereference(engine, iterator);
        if (iteratorResult.result == AspRunResult_IteratorAtEnd)
            break;
        if (iteratorResult.result != AspRunResult_OK)
            return iteratorResult.result;

        AspSequenceResult appendResult = AspSequenceAppend
            (engine, sequence, iteratorResult.value);
        if (appendResult.result != AspRunResult_OK)
            return appendResult.result;
        AspUnref(engine, iteratorResult.value);

        AspRunResult result = AspIteratorNext(engine, iterator);
        if (result != AspRunResult_OK)
            return iteratorResult.result;
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    AspUnref(engine, iterator);

    return AspRunResult_OK;
}
