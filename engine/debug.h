/*
 * Asp engine debug definitions.
 */

#ifndef ASP_DEBUG_H
#define ASP_DEBUG_H

#include "asp-priv.h"
#include "data.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t AspDataAddress(const AspEngine *, const AspDataEntry *);
uint32_t AspUseCount(const AspDataEntry *);
void AspTraceFile(AspEngine *, FILE *);
void AspDump(const AspEngine *, FILE *);

#ifdef __cplusplus
}
#endif

#endif
