/*
 * Asp engine iterator definitions.
 */

#ifndef ASP_ITERATOR_H
#define ASP_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"
#include "data.h"

typedef struct
{
    AspRunResult result;
    AspDataEntry *value;
} AspIteratorResult;

AspIteratorResult AspIteratorCreate
    (AspEngine *, AspDataEntry *iterable);
AspRunResult AspIteratorNext
    (AspEngine *, AspDataEntry *iterator);
AspIteratorResult AspIteratorDereference
    (AspEngine *, const AspDataEntry *iterator);

#ifdef __cplusplus
}
#endif

#endif
