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
     AspDataEntry *args, /* iterable group */
     AspDataEntry **returnValue)
{
    int32_t argCount;
    AspCount(engine, args, &argCount);
    if (argCount > 1)
        return AspRunResult_MalformedFunctionCall;
    AspDataEntry *iterable = argCount == 0 ? 0 : AspElement(engine, args, 0);

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
     AspDataEntry *args, /* iterable group */
     AspDataEntry **returnValue)
{
    int32_t argCount;
    AspCount(engine, args, &argCount);
    if (argCount > 1)
        return AspRunResult_MalformedFunctionCall;
    AspDataEntry *iterable = argCount == 0 ? 0 : AspElement(engine, args, 0);

    *returnValue = AspNewList(engine);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;

    return FillSequence(engine, *returnValue, iterable);
}

static AspRunResult FillSequence
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *iterable)
{
    if (iterable == 0)
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
