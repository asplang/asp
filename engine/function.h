/*
 * Asp engine function definitions.
 */

#ifndef ASP_FUNCTION_H
#define ASP_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"

AspRunResult AspExpandIterableGroupArgument
    (AspEngine *, AspDataEntry *argumentList, AspDataEntry *iterable);
AspRunResult AspExpandDictionaryGroupArgument
    (AspEngine *, AspDataEntry *argumentList, AspDataEntry *dictionary);
AspRunResult AspLoadArguments
    (AspEngine *,
     AspDataEntry *argumentList, AspDataEntry *parameterList,
     AspDataEntry *ns);

#ifdef __cplusplus
}
#endif

#endif
