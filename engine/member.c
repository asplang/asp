/*
 * Asp engine member lookup implementation.
 */

#include "member.h"
#include "tree.h"

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
    AspDataEntry *originalContainer = container;
    uint8_t originalContainerType = AspDataGetType(originalContainer);
    bool createAddress = address;
    AspDataEntry *foundMember = 0;
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
        foundMember = createAddress ? memberResult.node : memberResult.value;
        if (container == originalContainer)
            result.member = foundMember;

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

    bool newMemberValue = false;

    #ifdef ASP_FEATURE_CLASS

    /* Handle cases where a member is found in an inherited container (i.e., an
       instances's class or a class' base). */
    if (foundMember != 0 && container != originalContainer)
    {
        if (address)
        {
            /* Create a shadowing member referring to both the target and the
               found member value. */
            AspDataEntry *shadowingMember = AspAllocEntry
                (engine, DataType_ShadowingMember);
            AspDataSetShadowingMemberTargetIndex
                (shadowingMember, AspIndex(engine, result.member));
            AspRef(engine, foundMember);
            AspDataSetShadowingMemberSourceIndex
                (shadowingMember, AspIndex(engine, foundMember));
            result.member = shadowingMember;
            newMemberValue = true;
        }
        else if ((originalContainerType == DataType_Object ||
                  originalContainerType == DataType_Super) &&
                 AspDataGetType(foundMember) == DataType_Function)
        {
            AspDataEntry *instance = originalContainer;
            if (originalContainerType == DataType_Super)
            {
                /* Extract the underlying instance from the super object. */
                instance = AspValueEntry
                    (engine, AspDataGetSuperInstanceIndex(originalContainer));
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
            AspRef(engine, foundMember);
            AspDataSetBoundMethodFunctionIndex
                (boundMethod, AspIndex(engine, foundMember));
            AspRef(engine, instance);
            AspDataSetBoundMethodInstanceIndex
                (boundMethod, AspIndex(engine, instance));
            AspRef(engine, container);
            AspDataSetBoundMethodClassIndex
                (boundMethod, AspIndex(engine, container));
            result.member = boundMethod;
            newMemberValue = true;
        }
        else
            result.member = foundMember;
    }

    #endif

    if (!newMemberValue)
        AspRef(engine, result.member);

    return result;
}
