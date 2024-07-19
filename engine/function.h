/*
 * Asp engine function definitions.
 */

#ifndef ASP_FUNCTION_H
#define ASP_FUNCTION_H

#include "asp-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

AspRunResult AspExpandIterableGroupArgument
    (AspEngine *, AspDataEntry *argumentList, AspDataEntry *iterable);
AspRunResult AspExpandDictionaryGroupArgument
    (AspEngine *, AspDataEntry *argumentList, AspDataEntry *dictionary);
AspRunResult AspCallFunction
    (AspEngine *, AspDataEntry *function, AspDataEntry *argumentList,
     bool fromApp);
AspRunResult AspLoadArguments
    (AspEngine *,
     AspDataEntry *argumentList, AspDataEntry *parameterList,
     AspDataEntry *ns);
AspRunResult AspReturnToCaller(AspEngine *);

#ifdef __cplusplus
}
#endif

#endif
