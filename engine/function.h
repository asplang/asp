/*
 * Asp engine function definitions.
 */

#ifndef ASP_FUNCTION_H
#define ASP_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"

AspRunResult AspInitializeAppFunctions(AspEngine *);
AspRunResult AspLoadArguments
    (AspEngine *,
     AspDataEntry *argumentList, AspDataEntry *parameterList,
     AspDataEntry *ns);

#ifdef __cplusplus
}
#endif

#endif
