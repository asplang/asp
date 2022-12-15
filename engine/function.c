/*
 * Asp engine function implementation.
 */

#include "function.h"
#include "symbols.h"
#include "asp-priv.h"
#include "sequence.h"
#include "tree.h"
#include "data.h"

#ifdef ASP_DEBUG
#include <stdio.h>
#endif

static const uint32_t ParameterSpecMask = 0x0FFFFFFF;
static const uint32_t ParameterFlag_HasDefault = 0x10000000;
static const uint32_t ParameterFlag_IsGroup    = 0x20000000;
enum ParameterDefaultValueType
{
    ParameterDefaultValueType_None,
    ParameterDefaultValueType_Ellipsis,
    ParameterDefaultValueType_Boolean,
    ParameterDefaultValueType_Integer,
    ParameterDefaultValueType_Float,
    ParameterDefaultValueType_String,
};

AspRunResult AspInitializeAppFunctions(AspEngine *engine)
{
    /* Create definitions for application functions.
       Note that the first few symbols are reserved:
       0 - main module name
       1 - args*/
    unsigned specIndex = 0;
    for (int32_t functionSymbol = AspScriptSymbolBase; ; functionSymbol++)
    {
        if (specIndex >= engine->appSpec->specSize)
            break;
        unsigned parameterCount = engine->appSpec->spec[specIndex++];

        AspDataEntry *parameters = AspAllocEntry
            (engine, DataType_ParameterList);
        if (parameters == 0)
            return AspRunResult_OutOfDataMemory;
        for (unsigned p = 0; p < parameterCount; p++)
        {
            uint32_t parameterSpec = 0;
            for (unsigned i = 0; i < 4; i++)
            {
                parameterSpec <<= 8;
                parameterSpec |= engine->appSpec->spec[specIndex++];
            }
            uint32_t parameterSymbol = parameterSpec & ParameterSpecMask;
            bool hasDefault = (parameterSpec & ParameterFlag_HasDefault) != 0;
            bool isGroup = (parameterSpec & ParameterFlag_IsGroup) != 0;

            AspDataEntry *parameter = AspAllocEntry
                (engine, DataType_Parameter);
            if (parameter == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetParameterSymbol(parameter, parameterSymbol);
            AspDataSetParameterHasDefault(parameter, hasDefault);
            AspDataSetParameterIsGroup(parameter, isGroup);

            if (hasDefault)
            {
                AspDataEntry *defaultValue = 0;

                uint32_t valueType = engine->appSpec->spec[specIndex++];
                switch (valueType)
                {
                    default:
                        return AspRunResult_InitializationError;

                    case ParameterDefaultValueType_None:
                        defaultValue = AspNewNone(engine);
                        break;

                    case ParameterDefaultValueType_Ellipsis:
                        defaultValue = AspNewEllipsis(engine);
                        break;

                    case ParameterDefaultValueType_Boolean:
                    {
                        uint8_t value = engine->appSpec->spec[specIndex++];
                        defaultValue = AspNewBoolean(engine, value != 0);
                        break;
                    }

                    case ParameterDefaultValueType_Integer:
                    {
                        uint32_t uValue = 0;
                        for (unsigned i = 0; i < 4; i++)
                        {
                            uValue <<= 8;
                            uValue |= engine->appSpec->spec[specIndex++];
                        }
                        int32_t value = *(int32_t *)&uValue;
                        defaultValue = AspNewInteger(engine, value);
                        break;
                    }

                    case ParameterDefaultValueType_Float:
                    {
                        static const uint16_t word = 1;
                        bool be = *(const char *)&word == 0;

                        uint8_t data[sizeof(double)];
                        for (unsigned i = 0; i < sizeof(double); i++)
                            data[be ? i : sizeof(double) - 1 - i] =
                                engine->appSpec->spec[specIndex++];
                        double value = *(double *)data;
                        defaultValue = AspNewFloat(engine, value);
                        break;
                    }

                    case ParameterDefaultValueType_String:
                    {
                        uint32_t valueSize = 0;
                        for (unsigned i = 0; i < 4; i++)
                        {
                            valueSize <<= 8;
                            valueSize |= engine->appSpec->spec[specIndex++];
                        }
                        defaultValue = AspNewString
                            (engine,
                             engine->appSpec->spec + specIndex, valueSize);
                        specIndex += valueSize;
                        break;
                    }
                }

                if (defaultValue == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetParameterDefaultIndex
                    (parameter, AspIndex(engine, defaultValue));
            }

            AspSequenceResult parameterResult = AspSequenceAppend
                (engine, parameters, parameter);
            if (parameterResult.result != AspRunResult_OK)
                return parameterResult.result;
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

    /* Ensure we read the application spec correctly. */
    if (specIndex != engine->appSpec->specSize)
        return AspRunResult_InitializationError;

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
