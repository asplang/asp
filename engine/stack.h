/*
 * Asp engine stack definitions.
 */

#ifndef ASP_STACK_H
#define ASP_STACK_H

#include "asp-priv.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

AspDataEntry *AspPush(AspEngine *, AspDataEntry *value);
AspDataEntry *AspPushNoUse(AspEngine *, const AspDataEntry *value);
AspDataEntry *AspTopValue(AspEngine *);
AspDataEntry *AspTopValue2(AspEngine *);
bool AspPop(AspEngine *);
bool AspPopNoErase(AspEngine *);

#ifdef __cplusplus
}
#endif

#endif
