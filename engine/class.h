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

typedef struct
{
    AspRunResult result;
    AspDataEntry *value;
} AspContextResult;

AspSuperResult AspSuperCreate
    (AspEngine *engine, AspDataEntry *cls, AspDataEntry *instance);
AspContextResult AspFindContext(AspEngine *engine);

#ifdef __cplusplus
}
#endif

#endif
