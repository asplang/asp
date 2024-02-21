/*
 * Asp engine iterator definitions.
 */

#ifndef ASP_ITERATOR_H
#define ASP_ITERATOR_H

#include "asp-priv.h"
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

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
