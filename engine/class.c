/*
 * Asp engine class implementation.
 */

#include "class.h"

AspSuperResult AspSuperCreate
    (AspEngine *engine, AspDataEntry *cls, AspDataEntry *instance)
{
    AspSuperResult result = {AspRunResult_OK, 0};

    if (cls != 0 || instance != 0)
    {
        /* Ensure the given class and instance are valid. */
        if (!AspIsClass(cls) || !AspIsInstance(instance) ||
            !AspIsTypeOf(engine, instance, cls))
        {
            result.result = AspRunResult_UnexpectedType;
            return result;
        }
    }
    else
    {
        /* Search for the enclosing frame with a context. */
        AspDataEntry *context = 0;
        uint32_t prevStackEntryIndex;
        uint32_t iterationCount = 0;
        for (AspDataEntry *stackEntry = engine->stackTop;
             iterationCount < engine->cycleDetectionLimit && stackEntry != 0;
             iterationCount++, stackEntry =
             ((prevStackEntryIndex =
               AspDataGetStackEntryPreviousIndex(stackEntry)) != 0 ?
              AspEntry(engine, prevStackEntryIndex) : 0))
        {
            AspDataEntry *entry = AspValueEntry
                (engine, AspDataGetStackEntryValueIndex(stackEntry));
            if (AspDataGetType(entry) == DataType_Frame &&
                AspDataGetStackEntryHasValue2(stackEntry))
            {
                context = AspEntry
                    (engine, AspDataGetStackEntryValue2Index(stackEntry));
                break;
            }
        }
        if (iterationCount >= engine->cycleDetectionLimit)
        {
            result.result = AspRunResult_CycleDetected;
            return result;
        }
        if (context == 0)
        {
            result.result = AspRunResult_InvalidContext;
            return result;
        }

        /* Use the class and instance from the context. */
        uint32_t classIndex = AspDataGetContextClassIndex(context);
        if (classIndex == 0)
        {
            result.result = AspRunResult_InternalError;
            return result;
        }
        else
            cls = AspValueEntry(engine, classIndex);
        uint32_t instanceIndex = AspDataGetContextInstanceIndex(context);
        if (instanceIndex != 0)
            instance = AspValueEntry(engine, instanceIndex);
    }

    /* Create the super object. */
    result.value = AspAllocEntry(engine, DataType_Super);
    if (result.value == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspRef(engine, cls);
    AspDataSetSuperClassIndex(result.value, AspIndex(engine, cls));
    AspRef(engine, instance);
    AspDataSetSuperInstanceIndex(result.value, AspIndex(engine, instance));

    return result;
}
