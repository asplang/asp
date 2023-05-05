/*
 * Asp script function library implementation: collection functions.
 */

#include "asp.h"
#include "range.h"
#include "sequence.h"
#include "tree.h"

static AspRunResult FillSequence
    (AspEngine *, AspDataEntry *sequence, AspDataEntry *iterable);

/* tuple(x)
 * Convert the iterable to a tuple.
 */
AspRunResult AspLib_tuple
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
AspRunResult AspLib_list
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
    if (AspIsRange(iterable))
    {
        int32_t start, end, step;
        AspGetRange(engine, iterable, &start, &end, &step);
        for (int32_t i = start; step < 0 ? i < end : i < end; i += step)
        {
            AspDataEntry *value = AspNewInteger(engine, i);
            if (value == 0)
                return AspRunResult_OutOfDataMemory;
            AspSequenceResult appendResult = AspSequenceAppend
                (engine, sequence, value);
            if (appendResult.result != AspRunResult_OK)
                return appendResult.result;
            AspUnref(engine, value);
        }
    }
    else if (AspIsString(iterable) || AspIsSequence(iterable))
    {
        for (AspSequenceResult nextResult =
             AspSequenceNext(engine, iterable, 0);
             nextResult.element != 0;
             nextResult = AspSequenceNext
                (engine, iterable, nextResult.element))
        {
            if (AspIsString(iterable))
            {
                AspDataEntry *fragment = nextResult.value;
                uint8_t fragmentSize =
                    AspDataGetStringFragmentSize(fragment);
                char *fragmentData =
                    AspDataGetStringFragmentData(fragment);

                for (uint8_t fragmentIndex = 0;
                     fragmentIndex < fragmentSize;
                     fragmentIndex++)
                {
                    AspDataEntry *value = AspNewString
                        (engine, fragmentData + fragmentIndex, 1);
                    AspSequenceResult appendResult = AspSequenceAppend
                        (engine, sequence, value);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;
                    AspUnref(engine, value);
                }
            }
            else
            {
                AspSequenceResult appendResult = AspSequenceAppend
                    (engine, sequence, nextResult.value);
                if (appendResult.result != AspRunResult_OK)
                    return appendResult.result;
            }
        }
    }
    else if (AspIsSet(iterable) || AspIsDictionary(iterable))
    {
        for (AspTreeResult nextResult =
             AspTreeNext(engine, iterable, 0, true);
             nextResult.node != 0;
             nextResult = AspTreeNext
                (engine, iterable, nextResult.node, true))
        {
            if (AspIsSet(iterable))
            {
                AspSequenceResult appendResult = AspSequenceAppend
                    (engine, sequence, nextResult.key);
                if (appendResult.result != AspRunResult_OK)
                    return appendResult.result;
            }
            else
            {
                AspDataEntry *value = AspNewTuple(engine);
                if (value == 0)
                    return AspRunResult_OutOfDataMemory;

                AspSequenceResult addKeyResult = AspSequenceAppend
                    (engine, value, nextResult.key);
                if (addKeyResult.result != AspRunResult_OK)
                    return addKeyResult.result;
                AspSequenceResult addValueResult = AspSequenceAppend
                    (engine, value, nextResult.value);
                if (addValueResult.result != AspRunResult_OK)
                    return addValueResult.result;

                AspSequenceResult appendResult = AspSequenceAppend
                    (engine, sequence, value);
                if (appendResult.result != AspRunResult_OK)
                    return appendResult.result;
                AspUnref(engine, value);
            }
        }
    }
    else if (!AspIsNone(iterable))
        return AspRunResult_UnexpectedType;

    return AspRunResult_OK;
}