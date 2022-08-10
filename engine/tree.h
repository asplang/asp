/*
 * Asp engine tree definitions.
 */

#ifndef ASP_TREE_H
#define ASP_TREE_H

#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *node, *key, *value;
    bool inserted; /* false if duplicate found */
} AspTreeResult;

AspTreeResult AspTreeInsert
    (AspEngine *, AspDataEntry *tree,
     AspDataEntry *key, AspDataEntry *value);
AspTreeResult AspTreeTryInsertBySymbol
    (AspEngine *, AspDataEntry *tree,
     int32_t symbol, AspDataEntry *value);
AspRunResult AspTreeEraseNode
    (AspEngine *, AspDataEntry *tree, AspDataEntry *keyNode,
     bool eraseKey, bool eraseValue);
AspTreeResult AspTreeFind
    (AspEngine *, AspDataEntry *tree, AspDataEntry *key);
AspTreeResult AspFindSymbol
    (AspEngine *, AspDataEntry *tree, int32_t symbol);
AspTreeResult AspTreeNext
    (AspEngine *, AspDataEntry *tree, AspDataEntry *node);
AspDataEntry *AspTreeNodeValue(AspEngine *, AspDataEntry *node);

#ifdef __cplusplus
}
#endif

#endif
