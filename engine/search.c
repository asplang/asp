/*
 * Asp engine search implementation.
 */

#include "search.h"
#include "stack.h"
#include "sequence.h"

AspRunResult AspSearchNestedSequence
    (AspEngine *engine, const AspDataEntry *sequence,
     AspRunResult (*valuePredicate)
        (const AspDataEntry *value, void *context, bool *done),
     bool (*sequencePredicate)(const AspDataEntry *sequence),
     void *context)
{
    /* Ensure the given sequence is of the desired type. */
    if (!sequencePredicate(sequence))
        return AspRunResult_UnexpectedType;

    /* Search the sequence breadth-first, avoiding recursion by using the
       engine's stack. */
    bool done = false;
    const AspDataEntry *startStackTop = engine->stackTop;
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        /* Search the current sequence, deferring any sub-sequences to a
           subsequent search. */
        uint32_t iterationCount = 0;
        for (AspSequenceResult nextResult = AspSequenceNext
                (engine, sequence, 0, true);
             iterationCount < engine->cycleDetectionLimit &&
             nextResult.element != 0;
             iterationCount++,
             nextResult = AspSequenceNext
                (engine, sequence, nextResult.element, true))
        {
            const AspDataEntry *item = nextResult.value;

            if (sequencePredicate(item))
            {
                if (AspPushNoUse(engine, item) == 0)
                    return AspRunResult_OutOfDataMemory;
            }
            else
            {
                AspRunResult valuePredicateResult = valuePredicate
                    (item, context, &done);
                if (valuePredicateResult != AspRunResult_OK)
                    return valuePredicateResult;
                if (done)
                    break;
            }
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;
        if (done)
            break;

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        /* Fetch the next sequence from the stack. */
        sequence = AspTopValue(engine);
        AspPopNoErase(engine);
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    /* Unwind the working stack if necessary. */
    if (engine->runResult == AspRunResult_OK)
    {
        uint32_t iterationCount = 0;
        for (;
             iterationCount < engine->cycleDetectionLimit &&
             engine->stackTop != startStackTop;
             iterationCount++)
        {
            AspPopNoErase(engine);
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;
    }

    return AspRunResult_OK;
}
