/*
 * Asp engine class definitions.
 */

#ifndef ASP_CLASS_H
#define ASP_CLASS_H

#include "asp-priv.h"
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *value;
} AspSuperResult;

AspSuperResult AspSuperCreate
    (AspEngine *engine, AspDataEntry *cls, AspDataEntry *instance);

#ifdef __cplusplus
}
#endif

#endif
