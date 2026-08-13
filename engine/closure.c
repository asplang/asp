/*
 * Asp engine closure implementation.
 */

#include "closure.h"
#include "asp-priv.h"
#include "tree.h"

AspRunResult AspReferenceNamespaceChain
    (AspEngine *engine, AspDataEntry *ns, bool reference)
{
    /* Increase or decrease the reference count, as requested, for each
       namespace in the chain, potentially destroying namespaces when
       unreferencing. */
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit && ns != 0;
         iterationCount++)
    {
        AspDataEntry *enclosingNamespace = AspEntry
            (engine, AspDataGetNamespaceEnclosingNamespaceIndex(ns));
        (reference ? AspRef : AspUnref)(engine, ns);
        if (engine->runResult != AspRunResult_OK)
            return engine->runResult;
        ns = enclosingNamespace;
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    return AspRunResult_OK;
}

AspTreeResult AspSearchNonlocal
    (AspEngine *engine, int32_t symbol, AspDataEntry **foundNamespace)
{
    /* Search for the variable, starting with the local namespace's enclosing
       namespace. */
    AspTreeResult findResult = {AspRunResult_OK, 0, 0, 0};
    uint32_t iterationCount = 0;
    for (AspDataEntry *ns = AspEntry
            (engine,
             AspDataGetNamespaceEnclosingNamespaceIndex
                (engine->localNamespace));
         iterationCount < engine->cycleDetectionLimit && ns != 0;
         iterationCount++,
         ns = AspEntry(engine, AspDataGetNamespaceEnclosingNamespaceIndex(ns)))
    {
        /* Look for the variable in the current namespace. */
        findResult = AspFindSymbol(engine, ns, symbol);
        if (findResult.result != AspRunResult_OK)
            return findResult;
        if (findResult.node != 0)
        {
            if (foundNamespace != 0)
                *foundNamespace = ns;
            break;
        }
    }
    if (iterationCount >= engine->cycleDetectionLimit)
    {
        findResult.result = AspRunResult_CycleDetected;
        return findResult;
    }

    return findResult;
}
