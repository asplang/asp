/*
 * Asp engine debug definitions.
 */

#ifndef ASP_DEBUG_H
#define ASP_DEBUG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"

void AspDump(const AspEngine *, FILE *);

#ifdef __cplusplus
}
#endif

#endif
