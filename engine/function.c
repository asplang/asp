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

    AspDataEntry *group = 0;

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

        AspDataEntry *parameter = 0;
        if (group == 0)
        {
            parameterResult = AspSequenceNext
                (engine, parameterList, parameterResult.element);
            if (parameterResult.element == 0)
            {
                #ifdef ASP_DEBUG
                puts("Too many arguments for function");
                #endif
                return AspRunResult_MalformedFunctionCall;
            }

            parameter = parameterResult.value;
            if (AspDataGetParameterIsGroup(parameter))
            {
                group = AspAllocEntry(engine, DataType_Tuple);
                if (group == 0)
                    return AspRunResult_OutOfDataMemory;
                AspTreeResult insertResult = AspTreeTryInsertBySymbol
                    (engine, ns, AspDataGetParameterSymbol(parameter), group);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
                AspUnref(engine, group);
                if (engine->runResult != AspRunResult_OK)
                    return engine->runResult;
            }
        }

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));
        if (group != 0)
        {
            AspSequenceResult appendResult = AspSequenceAppend
                (engine, group, value);
            if (appendResult.result != AspRunResult_OK)
                return appendResult.result;
        }
        else
        {
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, ns, AspDataGetParameterSymbol(parameter), value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
        }
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
            puts("Positional args follow named args");
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
                if (AspDataGetParameterIsGroup(parameter))
                {
                    #ifdef ASP_DEBUG
                    puts("Cannot assign to group parameter");
                    #endif
                    return AspRunResult_MalformedFunctionCall;
                }

                parameterFound = true;
                break;
            }
        }
        if (!parameterFound)
        {
            #ifdef ASP_DEBUG
            puts("No such named parameter");
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
            puts("Parameter already assigned to an argument");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, ns, argumentSymbol, value);
        if (insertResult.result != AspRunResult_OK)
            return insertResult.result;
    }

    /* Assign defaults values to remaining parameters. */
    parameterResult = AspSequenceNext(engine, parameterList, 0);
    for (; parameterResult.element != 0;
         parameterResult = AspSequenceNext
            (engine, parameterList, parameterResult.element))
    {
        AspDataEntry *parameter = parameterResult.value;
        int32_t parameterSymbol = AspDataGetParameterSymbol(parameter);

        if (AspDataGetParameterIsGroup(parameter))
        {
            if (group == 0)
            {
                group = AspAllocEntry(engine, DataType_Tuple);
                if (group == 0)
                    return AspRunResult_OutOfDataMemory;
                AspTreeResult insertResult = AspTreeTryInsertBySymbol
                    (engine, ns, parameterSymbol, group);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
                AspUnref(engine, group);
                if (engine->runResult != AspRunResult_OK)
                    return engine->runResult;
            }
        }
        else
        {
            AspTreeResult findResult = AspFindSymbol
                (engine, ns, parameterSymbol);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            if (findResult.node != 0)
                continue;

            if (!AspDataGetParameterHasDefault(parameter))
            {
                #ifdef ASP_DEBUG
                puts("Missing parameter that has no default");
                #endif
                return AspRunResult_MalformedFunctionCall;
            }

            AspDataEntry *value = AspValueEntry
                (engine, AspDataGetParameterDefaultIndex(parameter));
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, ns, parameterSymbol, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
        }
    }

    if (AspDataGetTreeCount(ns) != AspDataGetSequenceCount(parameterList))
    {
        #ifdef ASP_DEBUG
        puts("Not all parameters were assigned a value");
        #endif
        return AspRunResult_MalformedFunctionCall;
    }

    return AspRunResult_OK;
}

AspDataEntry *AspParameterValue
    (AspEngine *engine, AspDataEntry *ns, int32_t symbol)
{
    return AspFindSymbol(engine, ns, symbol).value;
}

AspParameterResult AspGroupParameterValue
    (AspEngine *engine, AspDataEntry *ns, int32_t symbol)
{
    AspParameterResult result = {AspRunResult_OK, 0};

    AspTreeResult findResult = AspFindSymbol(engine, ns, symbol);
    result.result = findResult.result;
    result.value = findResult.value;
    if (result.result != AspRunResult_OK)
        return result;

    /* Ensure the parameter value is a tuple. */
    if (result.value == 0 || AspDataGetType(result.value) != DataType_Tuple)
    {
        #ifdef ASP_DEBUG
        puts("Group parameter is not a tuple");
        #endif
        result.result = AspRunResult_InternalError;
        return result;
    }

    return result;
}
