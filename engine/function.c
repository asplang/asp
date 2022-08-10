/*
 * Asp engine function implementation.
 */

#include "function.h"
#include "asp-priv.h"
#include "sequence.h"
#include "tree.h"
#include "data.h"

#ifdef ASP_DEBUG
#include <stdio.h>
#endif

AspRunResult AspInitializeAppFunctions(AspEngine *engine)
{
    /* Create definitions for application functions. */
    unsigned i = 0;
    for (int32_t functionSymbol = 1; ; functionSymbol++)
    {
        if (i >= engine->appSpec->specSize)
            break;
        unsigned parameterCount = engine->appSpec->spec[i++];

        if (i + 4 * parameterCount > engine->appSpec->specSize)
            return AspRunResult_InitializationError;

        AspDataEntry *parameters = AspAllocEntry
            (engine, DataType_ParameterList);
        if (parameters == 0)
            return AspRunResult_OutOfDataMemory;
        for (unsigned p = 0; p < parameterCount; p++)
        {
            uint32_t parameterSymbol = 0;
            for (unsigned j = 0; j < 4; j++)
            {
                parameterSymbol <<= 8;
                parameterSymbol |= engine->appSpec->spec[i++];
            }

            AspDataEntry *parameter = AspAllocEntry
                (engine, DataType_Parameter);
            if (parameter == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetParameterSymbol(parameter, parameterSymbol);
            AspDataSetParameterHasDefault(parameter, false);
            AspSequenceAppend(engine, parameters, parameter);
        }

        AspDataEntry *function = AspAllocEntry(engine, DataType_Function);
        if (function == 0)
            return AspRunResult_OutOfDataMemory;
        AspDataSetFunctionSymbol(function, functionSymbol);
        AspDataSetFunctionIsApp(function, true);
        AspRef(engine, engine->module);
        AspDataSetFunctionModuleIndex
            (function, AspIndex(engine, engine->module));
        AspDataSetFunctionParametersIndex
            (function, AspIndex(engine, parameters));

        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, engine->systemNamespace, functionSymbol, function);
        if (insertResult.result != AspRunResult_OK)
            return insertResult.result;
        if (!insertResult.inserted)
            return AspRunResult_InitializationError;
        AspUnref(engine, function);
    }

    return AspRunResult_OK;
}

/* Builds a namespace based on args and parameters. */
AspRunResult AspLoadArguments
    (AspEngine *engine,
     AspDataEntry *argumentList, AspDataEntry *parameterList,
     AspDataEntry *ns)
{
    AspAssert
        (engine,
         argumentList != 0 &&
         AspDataGetType(argumentList) == DataType_ArgumentList);
    AspAssert
        (engine,
         parameterList != 0 &&
         AspDataGetType(parameterList) == DataType_ParameterList);
    AspRunResult assertResult = AspAssert
        (engine,
         ns != 0 &&
         AspDataGetType(ns) == DataType_Namespace);

    AspSequenceResult argumentResult = AspSequenceNext(engine, argumentList, 0);

    /* Assign each positional argument to its matching parameter. */
    AspSequenceResult parameterResult = {AspRunResult_OK, 0, 0};
    for (; argumentResult.element != 0;
         argumentResult = AspSequenceNext
            (engine, argumentList, argumentResult.element))
    {
        AspDataEntry *argument = argumentResult.value;
        if (AspDataGetArgumentHasName(argument))
            break;

        parameterResult = AspSequenceNext
            (engine, parameterList, parameterResult.element);
        if (parameterResult.element == 0)
        {
            #ifdef ASP_DEBUG
            printf("Too many arguments for function\n");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }

        AspDataEntry *parameter = parameterResult.value;

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, ns, AspDataGetParameterSymbol(parameter), value);
        if (insertResult.result != AspRunResult_OK)
            return insertResult.result;
    }

    /* Assign each named argument to its matching parameter. */
    for (; argumentResult.element != 0;
         argumentResult = AspSequenceNext
            (engine, argumentList, argumentResult.element))
    {
        AspDataEntry *argument = argumentResult.value;

        if (!AspDataGetArgumentHasName(argument))
        {
            #ifdef ASP_DEBUG
            printf("Positional args follow named args\n");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }
        int32_t argumentSymbol = AspDataGetArgumentSymbol(argument);

        AspSequenceResult parameterResult = AspSequenceNext
            (engine, parameterList, 0);
        bool parameterFound = false;
        for (; parameterResult.element != 0;
             parameterResult = AspSequenceNext
                (engine, parameterList, parameterResult.element))
        {
            AspDataEntry *parameter = parameterResult.value;

            int32_t parameterSymbol = AspDataGetParameterSymbol(parameter);
            if (parameterSymbol == argumentSymbol)
            {
                parameterFound = true;
                break;
            }
        }
        if (!parameterFound)
        {
            #ifdef ASP_DEBUG
            printf("No such named parameter\n");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }

        AspTreeResult findResult = AspFindSymbol
            (engine, ns, argumentSymbol);
        if (findResult.result != AspRunResult_OK)
            return findResult.result;
        if (findResult.node != 0)
        {
            #ifdef ASP_DEBUG
            printf("Parameter already assigned to an argument\n");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, ns, argumentSymbol, value);
    }

    /* Assign defaults values to remaining parameters. */
    parameterResult = AspSequenceNext(engine, parameterList, 0);
    for (; parameterResult.element != 0;
         parameterResult = AspSequenceNext
            (engine, parameterList, parameterResult.element))
    {
        AspDataEntry *parameter = parameterResult.value;
        int32_t parameterSymbol = AspDataGetParameterSymbol(parameter);

        AspTreeResult findResult = AspFindSymbol
            (engine, ns, parameterSymbol);
        if (findResult.result != AspRunResult_OK)
            return findResult.result;
        if (findResult.node != 0)
            continue;

        if (!AspDataGetParameterHasDefault(parameter))
        {
            #ifdef ASP_DEBUG
            printf("Missing parameter that has no default\n");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetParameterDefaultIndex(parameter));
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, ns, parameterSymbol, value);
    }

    if (AspDataGetTreeCount(ns) != AspDataGetSequenceCount(parameterList))
    {
        #ifdef ASP_DEBUG
        printf("Not all parameters were assigned a value\n");
        #endif
        return AspRunResult_MalformedFunctionCall;
    }

    return AspRunResult_OK;
}

AspDataEntry *AspParameterValue
    (AspEngine *engine, AspDataEntry *ns, int32_t symbol)
{
    return AspTreeNodeValue
        (engine,
         AspFindSymbol(engine, ns, symbol).node);
}
