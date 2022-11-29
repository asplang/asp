/*
 * Asp engine assignment definitions.
 */

#ifndef ASP_ASSIGN_H
#define ASP_ASSIGN_H

#include "asp-priv.h"
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

AspRunResult AspAssignSimple
    (AspEngine *, AspDataEntry *address, AspDataEntry *newValue);
AspRunResult AspAssignSequence
    (AspEngine *, AspDataEntry *address, AspDataEntry *newValue);

#ifdef __cplusplus
}
#endif

#endif
