/*
 * Asp engine closure definitions.
 */

#ifndef ASP_CLOSURE_H
#define ASP_CLOSURE_H

#include "asp-priv.h"
#include "data.h"
#include "tree.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

AspRunResult AspProcessClosures(AspEngine *, AspDataEntry *ns);
AspRunResult AspTrackClosure(AspEngine *, AspDataEntry *);
AspRunResult AspReferenceNamespaceChain
    (AspEngine *, AspDataEntry *ns, bool reference);
AspTreeResult AspSearchNonlocal
    (AspEngine *, int32_t symbol, AspDataEntry **foundNamespace);

#ifdef __cplusplus
}
#endif

#endif
