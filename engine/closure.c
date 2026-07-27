/*
 * Asp engine closure implementation.
 */

#include "closure.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"
#include "assign.h"
#include "data.h"

static AspRunResult LocalizeClosures(AspEngine *, AspDataEntry *ns);
static AspRunResult LocalizeClosure
    (AspEngine *, AspDataEntry *ns,
     AspDataEntry *address, AspDataEntry *value);
static bool IsContainer(const AspDataEntry *);

AspRunResult AspProcessClosures(AspEngine *engine, AspDataEntry *ns)
{
    /* Attempt to make all closures within the namespace local to the
       namespace. The tracking list will contain any closures that were not
       completely local. */
    AspRunResult localizeResult = LocalizeClosures(engine, ns);
    if (localizeResult != AspRunResult_OK)
        return localizeResult;

    /* Detach any remaining closures. */
    if (engine->closureTrackers != 0)
    {
        uint32_t iterationCount = 0;
        for (AspSequenceResult nextResult = {AspRunResult_OK, 0, 0};
             iterationCount < engine->cycleDetectionLimit &&
             (nextResult = AspSequenceNext
                (engine, engine->closureTrackers, 0, true)).element != 0;
             iterationCount++)
        {
                AspDataEntry *tracker = nextResult.value;
                AspDataEntry *closure = AspEntry
                     (engine, AspDataGetClosureTrackerClosureIndex(tracker));
                uint32_t closuresIndex = AspDataGetClosureTrackerListIndex
                    (tracker);

                /* Erase the tracking record. */
                bool eraseResult = AspSequenceEraseElement
                    (engine, engine->closureTrackers,
                     nextResult.element, false);
                if (!eraseResult)
                    return AspRunResult_InternalError;
                AspUnref(engine, tracker);

                /* Mark the closure as detached. */
                AspDataSetClosureTrackerElementIndex(closure, 0);
                AspDataSetClosureIsDetached(closure, true);

                /* Ripple through the closure's chain of nonlocal namespaces,
                   referencing each one on behalf of the detached closure. */
                AspRunResult referenceResult = AspReferenceNamespaceChain
                    (engine,
                     AspEntry
                        (engine,
                         AspDataGetClosureNonlocalNamespaceIndex(closure)),
                     true);
                if (referenceResult != AspRunResult_OK)
                    return referenceResult;
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;

        /* Destroy the closure trackers list. */
        AspUnref(engine, engine->closureTrackers);
        engine->closureTrackers = 0;
    }

    /* Ripple through the current chain of nonlocal namespaces, unreferencing
       each one on behalf of the call frame. */
    AspRunResult unreferenceResult = AspReferenceNamespaceChain
        (engine,
         AspEntry(engine, AspDataGetNamespaceEnclosingNamespaceIndex(ns)),
         false);
    if (unreferenceResult != AspRunResult_OK)
        return unreferenceResult;

    return AspRunResult_OK;
}

static AspRunResult LocalizeClosures(AspEngine *engine, AspDataEntry *ns)
{
    /* Process all closures within the namespace, dispositioning them as
       applicable, avoiding recursion by using the engine's stack. */
    const AspDataEntry *startStackTop = engine->stackTop;
    uint32_t iterationCount = 0;
    for (AspDataEntry *entry = ns;
         iterationCount < engine->cycleDetectionLimit; iterationCount++)
    {
        uint8_t type = AspDataGetType(entry);
        switch (type)
        {
            case DataType_Tuple:
            case DataType_List:
            {
                /* Search the current sequence, deferring any sub-structures to
                   a subsequent search. */
                uint32_t iterationCount = 0;
                for (AspSequenceResult nextResult = AspSequenceNext
                        (engine, entry, 0, true);
                     iterationCount < engine->cycleDetectionLimit &&
                     nextResult.element != 0;
                     iterationCount++,
                     nextResult = AspSequenceNext
                        (engine, entry, nextResult.element, true))
                {
                    AspDataEntry *item = nextResult.value;

                    if (IsContainer(item))
                    {
                        if (AspPushNoUse(engine, item) == 0)
                            return AspRunResult_OutOfDataMemory;
                    }
                    else
                    {
                        AspRunResult localizeResult = LocalizeClosure
                            (engine, ns, nextResult.element, item);
                        if (localizeResult != AspRunResult_OK)
                            return localizeResult;
                    }
                }
                if (iterationCount >= engine->cycleDetectionLimit)
                    return AspRunResult_CycleDetected;

                break;
            }

            case DataType_Set:
            case DataType_Dictionary:
            case DataType_Object:
            #ifdef ASP_FEATURE_CLASS
            case DataType_Class:
            #endif
            case DataType_Namespace:
            {
                if (type == DataType_Object)
                {
                    entry = AspValueEntry
                        (engine, AspDataGetObjectNamespaceIndex(entry));
                    type = AspDataGetType(entry);
                }
                #ifdef ASP_FEATURE_CLASS
                else if (type == DataType_Class)
                {
                    entry = AspValueEntry
                        (engine, AspDataGetClassNamespaceIndex(entry));
                    type = AspDataGetType(entry);
                }
                #endif

                /* Search the current tree, deferring any sub-structures to a
                   subsequent search. */
                uint32_t iterationCount = 0;
                for (AspTreeResult nextResult = AspTreeNext
                        (engine, entry, 0, true);
                     iterationCount < engine->cycleDetectionLimit &&
                     nextResult.node != 0;
                     iterationCount++,
                     nextResult = AspTreeNext
                        (engine, entry, nextResult.node, true))
                {
                    AspDataEntry *value = nextResult.value;

                    if (value != 0)
                    {
                        if (IsContainer(value))
                        {
                            if (AspPushNoUse(engine, value) == 0)
                                return AspRunResult_OutOfDataMemory;
                        }
                        else
                        {
                            AspRunResult localizeResult = LocalizeClosure
                                (engine, ns, nextResult.node, value);
                            if (localizeResult != AspRunResult_OK)
                                return localizeResult;
                        }
                    }
                }
                if (iterationCount >= engine->cycleDetectionLimit)
                    return AspRunResult_CycleDetected;

                break;
            }
        }

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        /* Fetch the next structure from the stack. */
        entry = AspTopValue(engine);
        AspPopNoErase(engine);
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;
    if (engine->runResult != AspRunResult_OK)
        return engine->runResult;

    return AspRunResult_OK;
}

static AspRunResult LocalizeClosure
    (AspEngine *engine, AspDataEntry *ns,
     AspDataEntry *address, AspDataEntry *value)
{
    if (AspDataGetType(value) != DataType_Closure)
        return AspRunResult_OK;
    AspDataEntry *closure = value;

    /* If the closure has already been detached, retrack it. */
    if (AspDataGetClosureIsDetached(closure))
    {
        /* Clone the closure. */
        AspDataEntry *clone = AspAllocEntry(engine, DataType_Closure);
        if (clone == 0)
            return AspRunResult_OutOfDataMemory;
        AspRef
            (engine,
             AspEntry(engine, AspDataGetClosureFunctionIndex(closure)));
        AspDataSetClosureFunctionIndex
            (clone, AspDataGetClosureFunctionIndex(closure));
        AspDataSetClosureNonlocalNamespaceIndex
            (clone, AspDataGetClosureNonlocalNamespaceIndex(closure));

        /* Replace the original closure with the clone in the local
           namespace. This may cause the original closure to be destroyed. */
        AspRunResult replaceResult = AspAssignSimple(engine, address, clone);
        if (replaceResult != AspRunResult_OK)
            return replaceResult;
        AspUnref(engine, clone);
        if (engine->runResult != AspRunResult_OK)
            return engine->runResult;

        /* Track the clone. */
        AspRunResult trackResult = AspTrackClosure(engine, clone);
        if (trackResult != AspRunResult_OK)
            return trackResult;
        closure = clone;
    }

    /* Handle a tracked closure as applicable. */
    uint32_t trackerElementIndex = AspDataGetClosureTrackerElementIndex
        (closure);
    if (trackerElementIndex != 0)
    {
        AspDataEntry *element = AspEntry(engine, trackerElementIndex);
        AspDataEntry *tracker = AspEntry
            (engine, AspDataGetElementValueIndex(element));

        /* Obtain the closure's clone. */
        uint32_t cloneIndex = AspDataGetClosureTrackerCloneIndex(tracker);
        AspDataEntry *clone;
        if (cloneIndex != 0)
            clone = AspValueEntry(engine, cloneIndex);
        else
        {
            /* Clone the closure. */
            clone = AspAllocEntry(engine, DataType_Closure);
            if (clone == 0)
                return AspRunResult_OutOfDataMemory;
            AspRef
                (engine,
                 AspEntry(engine, AspDataGetClosureFunctionIndex(closure)));
            AspDataSetClosureFunctionIndex
                (clone, AspDataGetClosureFunctionIndex(closure));
            AspDataSetClosureNonlocalNamespaceIndex
                (clone, AspDataGetClosureNonlocalNamespaceIndex(closure));
            AspDataSetClosureTrackerCloneIndex
                (tracker, AspIndex(engine, clone));
        }

        /* Replace the original closure with the clone in the local
           namespace. This may cause the original closure to be destroyed. */
        AspRunResult replaceResult = AspAssignSimple(engine, address, clone);
        if (replaceResult != AspRunResult_OK)
            return replaceResult;
        if (cloneIndex == 0)
        {
            AspUnref(engine, clone);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
        }
    }

    return AspRunResult_OK;
}

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

AspRunResult AspTrackClosure(AspEngine *engine, AspDataEntry *closure)
{
    /* Ensure the closure is not already been tracked. */
    if (AspDataGetClosureTrackerElementIndex(closure) != 0)
        return AspRunResult_InternalError;

    /* Create the tracker list if it does not already exist. */
    if (engine->closureTrackers == 0)
    {
        engine->closureTrackers = AspAllocEntry
            (engine, DataType_ClosureTrackerList);
        if (engine->closureTrackers == 0)
            return AspRunResult_OutOfDataMemory;
    }

    /* Create a tracking record for the closure. */
    AspDataEntry *tracker = AspAllocEntry(engine, DataType_ClosureTracker);
    if (tracker == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetClosureTrackerClosureIndex(tracker, AspIndex(engine, closure));
    AspDataSetClosureTrackerListIndex
        (tracker, AspIndex(engine, engine->closureTrackers));

    /* Add the tracking record to the list. */
    AspSequenceResult appendResult = AspSequenceAppend
        (engine, engine->closureTrackers, tracker);
    if (appendResult.result != AspRunResult_OK)
        return appendResult.result;

    /* Update the closure to point to the tracking element within the list. */
    AspDataSetClosureTrackerElementIndex
        (closure, AspIndex(engine, appendResult.element));

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

static bool IsContainer(const AspDataEntry *entry)
{
    switch (AspDataGetType(entry))
    {
        case DataType_Tuple:
        case DataType_List:
        case DataType_Set:
        case DataType_Dictionary:
        case DataType_Object:
        #ifdef ASP_FEATURE_CLASS
        case DataType_Class:
        #endif
        case DataType_Namespace:
            return true;
    }
    return false;
}
