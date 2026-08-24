/*
 * Asp script function library implementation: class support functions.
 */

#include "asp.h"
#include "class.h"
#include "sequence.h"
#include "search.h"
#include "member.h"
#include "reserved.h"

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

/* staticmethod.__init__(self, func)
 * Initialize a staticmethod function wrapper.
 */
ASP_LIB_API AspRunResult AspLib_staticmethod_init
    (AspEngine *engine,
     AspDataEntry *self, AspDataEntry *func,
     AspDataEntry **returnValue)
{
    if (!AspIsInstance(self))
        return AspRunResult_InvalidContext;

    bool insertResult = AspObjectInsert
        (engine, self, AspReservedSymbol_FunctionMember, func, false);
    return insertResult ? AspRunResult_OK : AspRunResult_OutOfDataMemory;
}

/* staticmethod.__get__(self, instance, owner)
 * Return the bare function instead of a bound method.
 */
ASP_LIB_API AspRunResult AspLib_staticmethod_get
    (AspEngine *engine,
     AspDataEntry *self, AspDataEntry *instance, AspDataEntry *owner,
     AspDataEntry **returnValue)
{
    AspMemberResult result = AspFindMember
        (engine, self, AspReservedSymbol_FunctionMember, false);
    if (result.result != AspRunResult_OK)
        return result.result;
    *returnValue = result.member;
    return AspRunResult_OK;
}

/* classmethod.__init__(self, func)
 * Initialize a classmethod function wrapper.
 */
ASP_LIB_API AspRunResult AspLib_classmethod_init
    (AspEngine *engine,
     AspDataEntry *self, AspDataEntry *func,
     AspDataEntry **returnValue)
{
    if (!AspIsInstance(self))
        return AspRunResult_InvalidContext;

    bool insertResult = AspObjectInsert
        (engine, self, AspReservedSymbol_FunctionMember, func, false);
    return insertResult ? AspRunResult_OK : AspRunResult_OutOfDataMemory;
}

/* classmethod.__get__(self, instance, owner)
 * Return a method with the function bound to the class instead of the
 * instance.
 */
ASP_LIB_API AspRunResult AspLib_classmethod_get
    (AspEngine *engine,
     AspDataEntry *self, AspDataEntry *instance, AspDataEntry *owner,
     AspDataEntry **returnValue)
{
    if (AspIsNone(owner))
        owner = AspInstanceClass(engine, instance);

    AspMemberResult result = AspFindMember
        (engine, self, AspReservedSymbol_FunctionMember, false);
    if (result.result != AspRunResult_OK)
        return result.result;

    AspContextResult contextResult = AspFindContext(engine);
    if (contextResult.result != AspRunResult_OK)
        return contextResult.result;

    /* Return a bound method, binding the owner class as the instance. */
    *returnValue = AspAllocEntry(engine, DataType_BoundMethod);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetBoundMethodFunctionIndex
        (*returnValue, AspIndex(engine, result.member));
    AspRef(engine, owner);
    AspDataSetBoundMethodInstanceIndex(*returnValue, AspIndex(engine, owner));
    AspDataEntry *cls =
        contextResult.value == 0 ? owner :
        AspEntry(engine, AspDataGetContextClassIndex(contextResult.value));
    AspRef(engine, cls);
    AspDataSetBoundMethodClassIndex(*returnValue, AspIndex(engine, cls));

    return AspRunResult_OK;
}
