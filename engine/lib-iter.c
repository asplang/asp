/*
 * Asp script function library implementation: iterator functions.
 */

#include "asp.h"
#include "iterator.h"

/* iter(iterable)
 * Return an iterator for a given iterable.
 */
ASP_LIB_API AspRunResult AspLib_iter
    (AspEngine *engine,
     AspDataEntry *iterable,
     AspDataEntry **returnValue)
{
    AspIteratorResult result = AspIteratorCreate(engine, iterable);
    if (result.result == AspRunResult_OK)
        *returnValue = result.value;
    return result.result;
}

/* next(iterator, end = None)
 * Return the next item from the iterator, returning the given end value
   if the iterator has reached its end.
 */
ASP_LIB_API AspRunResult AspLib_next
    (AspEngine *engine,
     AspDataEntry *iterator, AspDataEntry *end,
     AspDataEntry **returnValue)
{
    /* Dereference the iterator before advancing it. */
    AspIteratorResult dereferenceResult = AspIteratorDereference
        (engine, iterator);
    if (dereferenceResult.result == AspRunResult_IteratorAtEnd)
    {
        AspRef(engine, end);
        *returnValue = end;
        return AspRunResult_OK;
    }
    else if (dereferenceResult.result != AspRunResult_OK)
        return dereferenceResult.result;

    /* Store the dereferenced value. */
    *returnValue = dereferenceResult.value;

    /* Advance the iterator. */
    return AspIteratorNext(engine, iterator);
}
