/*
 * Asp engine object comparison definitions.
 */

#ifndef ASP_COMPARE_H
#define ASP_COMPARE_H

#include "asp-priv.h"
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

int AspCompare
    (AspEngine *engine,
     const AspDataEntry *leftEntry, const AspDataEntry *rightEntry);

#ifdef __cplusplus
}
#endif

#endif
