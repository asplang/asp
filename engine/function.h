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
    (AspEngine *, AspDataEntry *argumentList, const AspDataEntry *iterable);
AspRunResult AspExpandDictionaryGroupArgument
    (AspEngine *, AspDataEntry *argumentList, const AspDataEntry *dictionary);
AspRunResult AspCallFunction
    (AspEngine *, AspDataEntry *function, AspDataEntry *argumentList,
     bool fromApp);
AspRunResult AspLoadArguments
    (AspEngine *,
     const AspDataEntry *argumentList, const AspDataEntry *parameterList,
     AspDataEntry *ns);
AspRunResult AspReturnToCaller(AspEngine *);

#ifdef __cplusplus
}
#endif

#endif
