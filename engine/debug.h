/*
 * Asp engine debug definitions.
 */

#ifndef ASP_DEBUG_H
#define ASP_DEBUG_H

#include "asp-priv.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void AspTraceFile(AspEngine *, FILE *);
void AspDump(const AspEngine *, FILE *);

#ifdef __cplusplus
}
#endif

#endif
