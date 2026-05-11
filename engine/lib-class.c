/*
 * Asp script function library implementation: class support functions.
 */

#include "asp.h"
#include "data.h"
#include "stack.h"
#include "range.h"
#include "sequence.h"
#include "search.h"
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct
{
    AspEngine *engine;
    const AspDataEntry *object;
    bool found;
} TypePredicateContext;

static AspRunResult TypeOfPredicate
    (const AspDataEntry *type, void *context, bool *done);
static AspRunResult SubclassOfPredicate
    (const AspDataEntry *cls, void *context, bool *done);

/* object_class.__init__(self)
 * Do nothing.
 */
ASP_LIB_API AspRunResult AspLib_class_init
    (AspEngine *engine,
     AspDataEntry *self,
     AspDataEntry **returnValue)
{
    return AspRunResult_OK;
}

/* isinstance(object, type)
 * Return True if the object is of one of the given type(s).
 */
ASP_LIB_API AspRunResult AspLib_isinstance
    (AspEngine *engine,
     AspDataEntry *object, AspDataEntry *type,
     AspDataEntry **returnValue)
{
    bool is = false;
    if (AspIsClass(type) || AspIsType(type))
    {
        is = AspIsTypeOf(engine, object, type);
    }
    else
    {
        TypePredicateContext context =
        {
            .engine = engine,
            .object = object,
            .found = false,
        };
        AspRunResult searchResult = AspSearchNestedSequence
            (engine, type, TypeOfPredicate, AspIsSequence, &context);
        if (searchResult != AspRunResult_OK)
            return searchResult;
        is = context.found;
    }

    return
        (*returnValue = AspNewBoolean(engine, is)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

static AspRunResult TypeOfPredicate
    (const AspDataEntry *type, void *genericContext, bool *done)
{
    TypePredicateContext *context = (TypePredicateContext *)genericContext;
    if (!AspIsClass(type) && !AspIsType(type))
        return AspRunResult_UnexpectedType;
    if (AspIsTypeOf(context->engine, context->object, type))
    {
        context->found = true;
        *done = true;
    }
    return AspRunResult_OK;
}

/* issubclass(object, type)
 * Return True if the class is a subclass of one of the given class(es).
 */
ASP_LIB_API AspRunResult AspLib_issubclass
    (AspEngine *engine,
     AspDataEntry *class1, AspDataEntry *class2,
     AspDataEntry **returnValue)
{
    if (!AspIsClass(class1))
        return AspRunResult_UnexpectedType;

    bool is = false;
    if (AspIsClass(class2))
    {
        is = AspIsSubclassOf(engine, class1, class2);
    }
    else
    {
        TypePredicateContext context =
        {
            .engine = engine,
            .object = class1,
            .found = false,
        };
        AspRunResult searchResult = AspSearchNestedSequence
            (engine, class2, SubclassOfPredicate, AspIsSequence, &context);
        if (searchResult != AspRunResult_OK)
            return searchResult;
        is = context.found;
    }

    return
        (*returnValue = AspNewBoolean(engine, is)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

static AspRunResult SubclassOfPredicate
    (const AspDataEntry *cls, void *genericContext, bool *done)
{
    TypePredicateContext *context = (TypePredicateContext *)genericContext;
    if (!AspIsClass(cls))
        return AspRunResult_UnexpectedType;
    if (AspIsSubclassOf(context->engine, context->object, cls))
    {
        context->found = true;
        *done = true;
    }
    return AspRunResult_OK;
}

/* super(type, object)
 * Return a super object for calling methods in a base class.
 */
ASP_LIB_API AspRunResult AspLib_super
    (AspEngine *engine,
     AspDataEntry *cls, AspDataEntry *instance,
     AspDataEntry **returnValue)
{
    if (!AspIsNone(cls) || !AspIsNone(instance))
    {
        if (!AspIsClass(cls) || !AspIsInstance(instance) ||
            !AspIsTypeOf(engine, instance, cls))
            return AspRunResult_UnexpectedType;
    }
    else
    {
        /* Search for the enclosing frame. */
        uint32_t prevStackEntryIndex = AspDataGetStackEntryPreviousIndex
            (engine->stackTop);
        if (prevStackEntryIndex == 0)
            return AspRunResult_InvalidContext;
        AspDataEntry *frame = 0;
        uint32_t iterationCount = 0;
        AspDataEntry *stackTop = engine->stackTop;
        for (AspDataEntry *stackEntry = AspEntry(engine, prevStackEntryIndex);
             iterationCount < engine->cycleDetectionLimit && stackEntry != 0;
             iterationCount++, stackEntry =
             ((prevStackEntryIndex =
               AspDataGetStackEntryPreviousIndex(stackEntry)) != 0 ?
              AspEntry(engine, prevStackEntryIndex) : 0))
        {
            AspDataEntry *entry = AspValueEntry
                (engine, AspDataGetStackEntryValueIndex(stackEntry));
            if (AspDataGetType(entry) == DataType_Frame)
            {
                frame = entry;
                break;
            }
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;
        if (frame == 0)
            return AspRunResult_InvalidContext;
        uint32_t contextIndex = AspDataGetFrameContextIndex(frame);
        if (contextIndex == 0)
            return AspRunResult_InvalidContext;

        AspDataEntry *context = AspEntry(engine, contextIndex);
        uint32_t classIndex = AspDataGetContextClassIndex(context);
        if (classIndex == 0)
            return AspRunResult_InternalError;
        else
            cls = AspValueEntry(engine, classIndex);
        uint32_t instanceIndex = AspDataGetContextInstanceIndex(context);
        if (instanceIndex != 0)
            instance = AspValueEntry(engine, instanceIndex);
    }

    *returnValue = AspAllocEntry(engine, DataType_Super);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;
    AspRef(engine, cls);
    AspDataSetSuperClassIndex(*returnValue, AspIndex(engine, cls));
    AspRef(engine, instance);
    AspDataSetSuperInstanceIndex(*returnValue, AspIndex(engine, instance));

    return AspRunResult_OK;
}
