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
AspRunResult AspCall
    (AspEngine *, AspDataEntry *function, AspDataEntry *argumentList,
     uint32_t pc);
AspRunResult AspLoadArguments
    (AspEngine *,
     AspDataEntry *argumentList, AspDataEntry *parameterList,
     AspDataEntry *ns);

#ifdef __cplusplus
}
#endif

#endif
