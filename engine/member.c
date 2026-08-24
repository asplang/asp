/*
 * Asp engine member lookup implementation.
 */

#include "member.h"
#include "tree.h"
#include "reserved.h"
#include "function.h"
#include "sequence.h"
#include "stack.h"
#include <stddef.h>

AspMemberResult AspFindMember
    (AspEngine *engine, AspDataEntry *container, int32_t symbol, bool address)
{
    AspMemberResult result = {AspRunResult_OK, 0};

    result.result = AspAssert
        (engine, container != 0 && AspIsObject(container));
    if (result.result != AspRunResult_OK)
        return result;

    /* Search the container and, if applicable, its ancestors, for the
       member. */
    AspMemberSeekResult symbolResult = AspSeekMember
        (engine, container, symbol, address);
    if (symbolResult.result != AspRunResult_OK)
    {
        result.result = symbolResult.result;
        return result;
    }
    result.member = address ? symbolResult.address : symbolResult.value;

    #ifdef ASP_FEATURE_CLASS

    /* Check for the descriptor protocol if the member was an instance found in
       a class and the operation is a get. */
    uint8_t containerType = AspDataGetType(container);
    if (!address && AspIsClass(symbolResult.container) &&
        AspIsInstance(symbolResult.value))
    {
        /* Access the class of the instance, which may (or may not) be a
           descriptor (i.e., define descriptor methods). */
        AspDataEntry *descriptorClass = AspValueEntry
            (engine, AspDataGetObjectClassIndex(symbolResult.value));

        /* Check whether a special __get__ function is defined. */
        AspMemberSeekResult getterResult = AspSeekMember
            (engine, descriptorClass, AspReservedSymbol_GetMethod, false);
        if (getterResult.result != AspRunResult_OK)
        {
            result.result = getterResult.result;
            return result;
        }

        /* Invoke the special __get__ function if applicable. */
        if (getterResult.value != 0)
        {
            /* Determine whether the container is a super object. */
            AspDataEntry *superClass = 0, *superInstance = 0;
            AspDataEntry *superInstanceClass = 0;
            if (containerType == DataType_Super)
            {
                superClass = AspEntry
                    (engine, AspDataGetSuperClassIndex(container));
                superInstance = AspEntry
                    (engine, AspDataGetSuperInstanceIndex(container));
                if (AspDataGetType(superInstance) == DataType_Class)
                    superInstanceClass = superInstance;
            }

            /* Create a temporary bound method, binding the descriptor instance
               to the descriptor's __get__ method. */
            AspDataEntry *boundMethod = AspAllocEntry
                (engine, DataType_BoundMethod);
            AspRef(engine, getterResult.value);
            AspDataSetBoundMethodFunctionIndex
                (boundMethod, AspIndex(engine, getterResult.value));
            AspRef(engine, symbolResult.value);
            AspDataSetBoundMethodInstanceIndex
                (boundMethod, AspIndex(engine, symbolResult.value));
            AspRef(engine, symbolResult.container);
            AspDataSetBoundMethodClassIndex
                (boundMethod, AspIndex(engine, symbolResult.container));

            /* Prepare to add arguments to the call. */
            AspDataEntry *arguments = AspAllocEntry
                (engine, DataType_ArgumentList);
            if (arguments == 0)
            {
                result.result = AspRunResult_OutOfDataMemory;
                return result;
            }

            /* Add the applicable instance argument. */
            bool instanceReferenced = false;
            AspDataEntry *instanceArgument =
                AspIsClass(container) || superInstanceClass != 0 ?
                (instanceReferenced = true, AspNewNone(engine)) :
                superInstance != 0 ? superInstance : container;
            if (!instanceReferenced)
                AspRef(engine, instanceArgument);
            result.result = AspAppendPositionalArgument
                (engine, arguments, instanceArgument);
            if (result.result != AspRunResult_OK)
                return result;

            /* Add the applicable owner argument. */
            AspDataEntry *ownerArgument =
                superInstanceClass != 0 ? superInstanceClass :
                AspIsInstance(instanceArgument) ?
                AspEntry(engine, AspDataGetObjectClassIndex(instanceArgument)) :
                superClass != 0 ? symbolResult.container : container;
            AspRef(engine, ownerArgument);
            result.result = AspAppendPositionalArgument
                (engine, arguments, ownerArgument);
            if (result.result != AspRunResult_OK)
                return result;

            /* Call the special __get__ method. */
            result.result = AspCallCallable
                (engine, boundMethod, arguments, engine->inApp);
            if (result.result == AspRunResult_Call)
                result.result = AspRunResult_NotImplemented;
            if (result.result != AspRunResult_OK)
                return result;

            /* We're now done with the bound method. */
            AspUnref(engine, boundMethod);

            /* Retain the returned value as the member value. */
            result.member = AspTopValue(engine);
            AspRef(engine, result.member);
            AspPop(engine);
            return result;
        }
    }

    #endif

    bool newMemberValue = false;

    #ifdef ASP_FEATURE_CLASS

    /* Handle cases where a member is found in an inherited container (i.e., an
       instances's class or a class' base). */
    if ((symbolResult.address != 0 || symbolResult.value != 0) &&
        symbolResult.container != container)
    {
        if (address)
        {
            /* Create a shadowing member referring to both the target and the
               found member value. */
            AspDataEntry *shadowingMember = AspAllocEntry
                (engine, DataType_ShadowingMember);
            AspDataSetShadowingMemberTargetIndex
                (shadowingMember, AspIndex(engine, symbolResult.address));
            AspRef(engine, symbolResult.value);
            AspDataSetShadowingMemberSourceIndex
                (shadowingMember, AspIndex(engine, symbolResult.value));
            result.member = shadowingMember;
            newMemberValue = true;
        }
        else if ((containerType == DataType_Object ||
                  containerType == DataType_Super) &&
                 AspDataGetType(symbolResult.value) == DataType_Function)
        {
            AspDataEntry *instance = container;
            if (containerType == DataType_Super)
            {
                /* Extract the underlying instance from the super object. */
                instance = AspValueEntry
                    (engine, AspDataGetSuperInstanceIndex(container));
                if (AspDataGetType(instance) != DataType_Object)
                {
                    result.result = AspRunResult_UnexpectedType;
                    return result;
                }
            }

            /* Create a bound method, binding the original instance to the
               found member function. */
            AspDataEntry *boundMethod = AspAllocEntry
                (engine, DataType_BoundMethod);
            AspRef(engine, symbolResult.value);
            AspDataSetBoundMethodFunctionIndex
                (boundMethod, AspIndex(engine, symbolResult.value));
            AspRef(engine, instance);
            AspDataSetBoundMethodInstanceIndex
                (boundMethod, AspIndex(engine, instance));
            AspRef(engine, symbolResult.container);
            AspDataSetBoundMethodClassIndex
                (boundMethod, AspIndex(engine, symbolResult.container));
            result.member = boundMethod;
            newMemberValue = true;
        }
        else
            result.member = symbolResult.value;
    }
    else
        result.member = address ?
            symbolResult.address : symbolResult.value;

    #endif

    if (result.member != 0 && !newMemberValue)
        AspRef(engine, result.member);

    return result;
}

AspMemberSeekResult AspSeekMember
    (AspEngine *engine, AspDataEntry *container, int32_t symbol, bool address)
{
    AspMemberSeekResult result = {AspRunResult_OK, 0, 0, 0};

    result.result = AspAssert
        (engine, container != 0 && AspIsObject(container));
    if (result.result != AspRunResult_OK)
        return result;

    /* Search the container and, if applicable, its ancestors, for the given
       members. */
    bool createAddress = address;
    #ifdef ASP_FEATURE_CLASS
    uint32_t iterationCount = 0;
    for (; iterationCount < engine->cycleDetectionLimit;
         iterationCount++)
    #endif
    {
        /* Access the container's namespace. */
        AspDataEntry *ns = 0;
        uint8_t containerType = AspDataGetType(container);
        switch (containerType)
        {
            default:
                result.result = AspRunResult_UnexpectedType;
                return result;

            case DataType_Object:
                ns = AspEntry
                    (engine, AspDataGetObjectNamespaceIndex(container));
                break;

            #ifdef ASP_FEATURE_CLASS

            case DataType_Class:
            {
                if (!AspIsFeature(engine, AspFeatureBit_Class))
                {
                    result.result = AspRunResult_UnexpectedType;
                    return result;
                }

                ns = AspEntry
                    (engine, AspDataGetClassNamespaceIndex(container));
                break;
            }

            case DataType_Super:
            {
                if (!AspIsFeature(engine, AspFeatureBit_Class))
                {
                    result.result = AspRunResult_UnexpectedType;
                    return result;
                }

                /* Start the search at the class' base class. */
                AspDataEntry *cls = AspValueEntry
                    (engine, AspDataGetSuperClassIndex(container));
                if (AspDataGetType(cls) != DataType_Class)
                {
                    result.result = AspRunResult_UnexpectedType;
                    return result;
                }
                uint32_t baseClassIndex = AspDataGetClassBaseClassIndex(cls);
                if (baseClassIndex == 0)
                    break;
                container = AspValueEntry(engine, baseClassIndex);
                containerType = AspDataGetType(container);
                ns = AspEntry
                    (engine, AspDataGetClassNamespaceIndex(container));
                break;
            }

            #endif

            case DataType_Module:
                ns = AspEntry
                    (engine, AspDataGetModuleNamespaceIndex(container));
                break;
        }
        if (ns == 0)
        {
            #ifdef ASP_FEATURE_CLASS
            break;
            #else
            result.result = AspRunResult_InternalError;
            return result;
            #endif
        }
        if (AspDataGetType(ns) != DataType_Namespace)
        {
            result.result = AspRunResult_UnexpectedType;
            return result;
        }

        /* Look up the variable in the namespace, creating it for an address
           lookup, if applicable, if it doesn't exist. */
        AspTreeResult memberResult = createAddress ?
            AspTreeTryInsertBySymbol
                (engine, ns, symbol, engine->noneSingleton) :
            AspFindSymbol(engine, ns, symbol);
        if (memberResult.result != AspRunResult_OK)
        {
            result.result = memberResult.result;
            return result;
        }
        if (createAddress)
            result.address = memberResult.node;
        else if (memberResult.value != 0)
            result.value = memberResult.value;
        if (memberResult.node != 0)
            result.container = container;

        #ifdef ASP_FEATURE_CLASS

        /* End the search if the member was found or the container is a module,
           in which case there's nowhere else to search. */
        if (containerType == DataType_Module ||
            memberResult.value != 0 && !memberResult.inserted)
            break;

        /* Keep searching. */
        uint32_t nextContainerIndex = 0;
        switch (containerType)
        {
            default:
                result.result = AspRunResult_InternalError;
                return result;

            case DataType_Object:
                nextContainerIndex = AspDataGetObjectClassIndex(container);
                break;

            case DataType_Class:
                nextContainerIndex = AspDataGetClassBaseClassIndex(container);
                break;
        }
        if (nextContainerIndex == 0)
            break;
        container = AspValueEntry(engine, nextContainerIndex);
        createAddress = false;

        #endif
    }
    #ifdef ASP_FEATURE_CLASS
    if (iterationCount >= engine->cycleDetectionLimit)
    {
        result.result = AspRunResult_CycleDetected;
        return result;
    }
    #endif

    return result;
}
