/*
 * Asp engine search definitions.
 */

#ifndef ASP_SEARCH_H
#define ASP_SEARCH_H

#include "asp-priv.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

AspRunResult AspSearchNestedSequence
    (AspEngine *, const AspDataEntry *sequence,
     AspRunResult (*valuePredicate)
        (const AspDataEntry *value, void *context, bool *done),
     bool (*sequencePredicate)(const AspDataEntry *sequence),
     void *context);

#ifdef __cplusplus
}
#endif

#endif
