/*
 * Asp script function library implementation: class support functions.
 */

#include "asp.h"
#include "class.h"
#include "sequence.h"
#include "search.h"

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
    AspSuperResult result = AspSuperCreate
        (engine, AspIsNone(cls) ? 0 : cls, AspIsNone(instance) ? 0 : instance);
    if (result.result != AspRunResult_OK)
        return result.result;
    *returnValue = result.value;
    return AspRunResult_OK;
}
