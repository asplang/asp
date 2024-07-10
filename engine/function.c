/*
 * Asp engine function implementation.
 */

#include "function.h"
#include "asp-priv.h"
#include "range.h"
#include "stack.h"
#include "sequence.h"
#include "tree.h"
#include "integer.h"
#include "integer-result.h"
#include "code.h"
#include "data.h"

#ifdef ASP_DEBUG
#include <stdio.h>
#endif

AspRunResult AspExpandIterableGroupArgument
    (AspEngine *engine, AspDataEntry *argumentList, AspDataEntry *iterable)
{
    switch (AspDataGetType(iterable))
    {
        default:
            return AspRunResult_UnexpectedType;

        case DataType_Range:
        {
            int32_t start, end, step;
            bool bounded;
            AspGetRange(engine, iterable, &start, &end, &step, &bounded);
            if (!bounded)
                return AspRunResult_ValueOutOfRange;
            AspRunResult stepResult = AspRunResult_OK;
            for (int32_t i = start;
                 stepResult == AspRunResult_OK && step < 0 ? i > end : i < end;
                 stepResult = AspTranslateIntegerResult
                    (AspAddIntegers(i, step, &i)))
            {
                /* Create an integer value from the range. */
                AspDataEntry *value = AspNewInteger(engine, i);
                if (value == 0)
                    return AspRunResult_OutOfDataMemory;

                /* Create an argument. */
                AspDataEntry *argument = AspAllocEntry
                    (engine, DataType_Argument);
                if (argument == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetArgumentValueIndex
                    (argument, AspIndex(engine, value));

                AspSequenceResult appendResult = AspSequenceAppend
                    (engine, argumentList, argument);
                if (appendResult.result != AspRunResult_OK)
                    return appendResult.result;
            }

            break;
        }

        case DataType_String:
        case DataType_Tuple:
        case DataType_List:
        {
            uint32_t iterationCount = 0;
            for (AspSequenceResult nextResult =
                 AspSequenceNext(engine, iterable, 0, true);
                 iterationCount < engine->cycleDetectionLimit &&
                 nextResult.element != 0;
                 iterationCount++,
                 nextResult = AspSequenceNext
                    (engine, iterable, nextResult.element, true))
            {
                AspDataEntry *value = nextResult.value;

                if (AspDataGetType(iterable) == DataType_String)
                {
                    AspDataEntry *fragment = nextResult.value;
                    uint8_t fragmentSize = AspDataGetStringFragmentSize
                        (fragment);
                    char *fragmentData = AspDataGetStringFragmentData
                        (fragment);

                    for (uint8_t fragmentIndex = 0;
                         fragmentIndex < fragmentSize;
                         fragmentIndex++)
                    {
                        /* Create a single-character string. */
                        AspDataEntry *value = AspNewString
                            (engine, fragmentData + fragmentIndex, 1);
                        if (value == 0)
                            return AspRunResult_OutOfDataMemory;

                        /* Create an argument. */
                        AspDataEntry *argument = AspAllocEntry
                            (engine, DataType_Argument);
                        if (argument == 0)
                            return AspRunResult_OutOfDataMemory;
                        AspDataSetArgumentValueIndex
                            (argument, AspIndex(engine, value));

                        /* Append the argument to the list. */
                        AspSequenceResult appendResult = AspSequenceAppend
                            (engine, argumentList, argument);
                        if (appendResult.result != AspRunResult_OK)
                            return appendResult.result;
                    }
                }
                else
                {
                    /* Use the sequence item as is. */
                    AspRef(engine, value);

                    /* Create an argument. */
                    AspDataEntry *argument = AspAllocEntry
                        (engine, DataType_Argument);
                    if (argument == 0)
                        return AspRunResult_OutOfDataMemory;
                    AspDataSetArgumentValueIndex
                        (argument, AspIndex(engine, value));

                    /* Append the argument to the list. */
                    AspSequenceResult appendResult = AspSequenceAppend
                        (engine, argumentList, argument);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;
                }
            }
            if (iterationCount >= engine->cycleDetectionLimit)
                return AspRunResult_CycleDetected;

            break;
        }

        case DataType_Set:
        case DataType_Dictionary:
        {
            uint32_t iterationCount = 0;
            for (AspTreeResult nextResult =
                 AspTreeNext(engine, iterable, 0, true);
                 iterationCount < engine->cycleDetectionLimit &&
                 nextResult.node != 0;
                 iterationCount++,
                 nextResult = AspTreeNext
                    (engine, iterable, nextResult.node, true))
            {
                AspDataEntry *value = nextResult.key;
                AspDataEntry *entryValue = nextResult.value;

                /* Use an appropriate argument value. */
                if (entryValue == 0)
                    AspRef(engine, value);
                else
                {
                    /* Create a key/value tuple. */
                    AspDataEntry *tuple = AspAllocEntry
                        (engine, DataType_Tuple);
                    if (tuple == 0)
                        return AspRunResult_OutOfDataMemory;
                    AspSequenceResult appendResult = AspSequenceAppend
                        (engine, tuple, value);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;
                    appendResult = AspSequenceAppend
                        (engine, tuple, entryValue);
                    if (appendResult.result != AspRunResult_OK)
                        return appendResult.result;
                    value = tuple;
                }

                /* Create an argument. */
                AspDataEntry *argument = AspAllocEntry
                    (engine, DataType_Argument);
                if (argument == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetArgumentValueIndex
                    (argument, AspIndex(engine, value));

                /* Append the argument to the list. */
                AspSequenceResult appendResult = AspSequenceAppend
                    (engine, argumentList, argument);
                if (appendResult.result != AspRunResult_OK)
                    return appendResult.result;
            }
            if (iterationCount >= engine->cycleDetectionLimit)
                return AspRunResult_CycleDetected;

            break;
        }
    }

    return AspRunResult_OK;
}

AspRunResult AspExpandDictionaryGroupArgument
    (AspEngine *engine, AspDataEntry *argumentList, AspDataEntry *dictionary)
{
    if (AspDataGetType(dictionary) != DataType_Dictionary)
        return AspRunResult_UnexpectedType;

    uint32_t iterationCount = 0;
    for (AspTreeResult nextResult =
         AspTreeNext(engine, dictionary, 0, true);
         iterationCount < engine->cycleDetectionLimit &&
         nextResult.node != 0;
         iterationCount++,
         nextResult = AspTreeNext
            (engine, dictionary, nextResult.node, true))
    {
        AspDataEntry *key = nextResult.key;
        AspDataEntry *value = nextResult.value;

        /* Ensure the key is a symbol. This will become the argument name. */
        if (AspDataGetType(key) != DataType_Symbol)
            return AspRunResult_UnexpectedType;
        int32_t symbol = AspDataGetSymbol(key);

        /* Use the entry value as is. */
        AspRef(engine, value);

        /* Create a named argument. */
        AspDataEntry *argument = AspAllocEntry
            (engine, DataType_Argument);
        if (argument == 0)
            return AspRunResult_OutOfDataMemory;
        AspDataSetArgumentSymbol(argument, symbol);
        AspDataSetArgumentHasName(argument, true);
        AspDataSetArgumentValueIndex(argument, AspIndex(engine, value));

        /* Append the argument to the list. */
        AspSequenceResult appendResult = AspSequenceAppend
            (engine, argumentList, argument);
        if (appendResult.result != AspRunResult_OK)
            return appendResult.result;
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    return AspRunResult_OK;
}

AspRunResult AspCall
    (AspEngine *engine, AspDataEntry *function, AspDataEntry *argumentList,
     uint32_t pc)
{
    AspDataEntry *ns = 0;
    if (!engine->again)
    {
        /* Gain access to the parameter list within the function. */
        AspDataEntry *parameters = AspEntry
            (engine, AspDataGetFunctionParametersIndex(function));
        if (AspDataGetType(parameters) != DataType_ParameterList)
            return AspRunResult_UnexpectedType;

        /* Create a local namespace for the call. */
        ns = AspAllocEntry(engine, DataType_Namespace);
        if (ns == 0)
            return AspRunResult_OutOfDataMemory;
        AspRunResult loadArgumentsResult = AspLoadArguments
            (engine, argumentList, parameters, ns);
        if (loadArgumentsResult != AspRunResult_OK)
            return loadArgumentsResult;
        AspUnref(engine, argumentList);
        if (engine->runResult != AspRunResult_OK)
            return engine->runResult;
    }

    /* Call the function. */
    if (engine->again || AspDataGetFunctionIsApp(function))
    {
        if (!engine->again)
        {
            engine->appFunctionSymbol = AspDataGetFunctionSymbol
                (function);
            engine->appFunctionNamespace = ns;
            engine->appFunctionReturnValue = 0;
        }

        /* Call the application function. */
        engine->inApp = true;
        AspRunResult callResult = engine->appSpec->dispatch
            (engine,
             engine->appFunctionSymbol, engine->appFunctionNamespace,
             &engine->appFunctionReturnValue);
        engine->inApp = false;
        if (callResult == AspRunResult_OK)
            engine->again = false;
        else if (callResult == AspRunResult_Again)
        {
            /* Cause this instruction to execute again. */
            engine->pc = pc;
            engine->again = true;
            callResult = AspRunResult_OK;
        }
        if (callResult != AspRunResult_OK)
        {
            if (callResult == AspRunResult_Complete)
                callResult = AspRunResult_InvalidAppFunction;
            return callResult;
        }

        if (!engine->again)
        {
            /* We're now done with the local namespace. */
            AspUnref(engine, engine->appFunctionNamespace);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
            engine->appFunctionSymbol = 0;
            engine->appFunctionNamespace = 0;

            /* Ensure there's a return value and push it onto the
               stack. */
            AspDataEntry *returnValue = engine->appFunctionReturnValue;
            engine->appFunctionReturnValue = 0;
            if (returnValue == 0)
            {
                returnValue = AspAllocEntry(engine, DataType_None);
                if (returnValue == 0)
                    return AspRunResult_OutOfDataMemory;
            }
            AspDataEntry *stackEntry = AspPush(engine, returnValue);
            if (stackEntry == 0)
                return AspRunResult_OutOfDataMemory;
            AspUnref(engine, returnValue);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
        }
    }
    else
    {
        /* Obtain the code address of the script-defined function. */
        uint32_t codeAddress = AspDataGetFunctionCodeAddress(function);
        AspRunResult validateResult = AspValidateCodeAddress
            (engine, codeAddress);
        if (validateResult != AspRunResult_OK)
            return validateResult;

        /* Create a new frame and push it onto the stack. */
        AspDataEntry *frame = AspAllocEntry(engine, DataType_Frame);
        if (frame == 0)
            return AspRunResult_OutOfDataMemory;
        AspDataSetFrameReturnAddress
            (frame, (uint32_t)AspProgramCounter(engine));
        AspRef(engine, engine->module);
        AspDataSetFrameModuleIndex
            (frame, AspIndex(engine, engine->module));
        AspDataSetFrameLocalNamespaceIndex
            (frame, AspIndex(engine, engine->localNamespace));
        AspDataEntry *newTop = AspPush(engine, frame);
        if (newTop == 0)
            return AspRunResult_OutOfDataMemory;

        /* Replace the current module and global namespace with
           those of the function. */
        AspDataEntry *functionModule = AspValueEntry
            (engine, AspDataGetFunctionModuleIndex(function));
        engine->module = functionModule;
        engine->globalNamespace = AspEntry
            (engine, AspDataGetModuleNamespaceIndex(functionModule));

        /* Replace the current local namespace with function's new
           namespace. */
        engine->localNamespace = ns;

        /* Transfer control to the function's code. */
        engine->pc = codeAddress;
    }

    return AspRunResult_OK;
}

/* Builds a namespace based on arguments and parameters. */
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
    if (assertResult != AspRunResult_OK)
        return assertResult;

    AspDataEntry *tupleGroup = 0;

    AspSequenceResult argumentResult = AspSequenceNext
        (engine, argumentList, 0, true);

    /* Assign each positional argument to its matching parameter. */
    AspSequenceResult parameterResult = {AspRunResult_OK, 0, 0};
    uint32_t iterationCount = 0;
    for (;
         iterationCount < engine->cycleDetectionLimit &&
         argumentResult.element != 0;
         iterationCount++,
         argumentResult = AspSequenceNext
            (engine, argumentList, argumentResult.element, true))
    {
        AspDataEntry *argument = argumentResult.value;
        if (AspDataGetArgumentHasName(argument) ||
            AspDataGetArgumentIsDictionaryGroup(argument))
            break;

        AspDataEntry *parameter = 0;
        if (tupleGroup == 0)
        {
            parameterResult = AspSequenceNext
                (engine, parameterList, parameterResult.element, true);
            if (parameterResult.element == 0)
            {
                #ifdef ASP_DEBUG
                puts("Too many arguments for function");
                #endif
                return AspRunResult_MalformedFunctionCall;
            }

            parameter = parameterResult.value;
            if (AspDataGetParameterIsTupleGroup(parameter))
            {
                tupleGroup = AspAllocEntry(engine, DataType_Tuple);
                if (tupleGroup == 0)
                    return AspRunResult_OutOfDataMemory;
                AspTreeResult insertResult = AspTreeTryInsertBySymbol
                    (engine, ns, AspDataGetParameterSymbol(parameter),
                     tupleGroup);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
                AspUnref(engine, tupleGroup);
                if (engine->runResult != AspRunResult_OK)
                    return engine->runResult;
            }
        }

        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));
        if (tupleGroup != 0)
        {
            AspSequenceResult appendResult = AspSequenceAppend
                (engine, tupleGroup, value);
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
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    /* Determine whether there is a dictionary group parameter. */
    AspDataEntry *dictionaryGroup = 0;
    AspSequenceResult lastParameterResult = AspSequenceNext
        (engine, parameterList, 0, false);
    if (lastParameterResult.element != 0 &&
        AspDataGetParameterIsDictionaryGroup(lastParameterResult.value))
    {
        AspDataEntry *parameter = lastParameterResult.value;
        dictionaryGroup = AspAllocEntry(engine, DataType_Dictionary);
        if (dictionaryGroup == 0)
            return AspRunResult_OutOfDataMemory;
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, ns, AspDataGetParameterSymbol(parameter),
             dictionaryGroup);
        if (insertResult.result != AspRunResult_OK)
            return insertResult.result;
        AspUnref(engine, dictionaryGroup);
        if (engine->runResult != AspRunResult_OK)
            return engine->runResult;
    }

    /* Assign each named argument to its matching parameter. */
    iterationCount = 0;
    for (;
         iterationCount < engine->cycleDetectionLimit &&
         argumentResult.element != 0;
         iterationCount++,
         argumentResult = AspSequenceNext
            (engine, argumentList, argumentResult.element, true))
    {
        AspDataEntry *argument = argumentResult.value;
        AspDataEntry *value = AspValueEntry
            (engine, AspDataGetArgumentValueIndex(argument));

        if (!AspDataGetArgumentHasName(argument))
        {
            #ifdef ASP_DEBUG
            puts("Positional arguments follow named arguments");
            #endif
            return AspRunResult_MalformedFunctionCall;
        }
        int32_t argumentSymbol = AspDataGetArgumentSymbol(argument);

        AspSequenceResult parameterResult = AspSequenceNext
            (engine, parameterList, 0, true);
        bool parameterFound = false;
        uint32_t iterationCount = 0;
        for (;
             iterationCount < engine->cycleDetectionLimit &&
             parameterResult.element != 0;
             iterationCount++,
             parameterResult = AspSequenceNext
                (engine, parameterList, parameterResult.element, true))
        {
            AspDataEntry *parameter = parameterResult.value;

            int32_t parameterSymbol = AspDataGetParameterSymbol(parameter);
            if (parameterSymbol == argumentSymbol)
            {
                if (AspDataGetParameterIsTupleGroup(parameter))
                {
                    #ifdef ASP_DEBUG
                    puts("Cannot assign to tuple group parameter");
                    #endif
                    return AspRunResult_MalformedFunctionCall;
                }
                else if (AspDataGetParameterIsDictionaryGroup(parameter))
                {
                    #ifdef ASP_DEBUG
                    puts("Cannot assign to dictionary group parameter");
                    #endif
                    return AspRunResult_MalformedFunctionCall;
                }

                parameterFound = true;
                break;
            }
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;
        if (!parameterFound)
        {
            if (dictionaryGroup == 0)
            {
                #ifdef ASP_DEBUG
                puts("No such named parameter and no dictionary group");
                #endif
                return AspRunResult_MalformedFunctionCall;
            }

            /* Create a symbol object key for adding the argument value to the
               dictionary group. */
            AspDataEntry *key = AspAllocEntry(engine, DataType_Symbol);
            if (key == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetSymbol(key, argumentSymbol);

            /* Ensure the key is not a duplicate. */
            AspTreeResult findResult = AspTreeFind
                (engine, dictionaryGroup, key);
            if (findResult.result != AspRunResult_OK)
                return findResult.result;
            if (findResult.node != 0)
            {
                #ifdef ASP_DEBUG
                puts("Name already present in the dictionary group");
                #endif
                return AspRunResult_MalformedFunctionCall;
            }

            /* Add the named argument to the dictionary group. */
            AspTreeResult insertResult = AspTreeInsert
                (engine, dictionaryGroup, key, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            AspUnref(engine, key);
            if (engine->runResult != AspRunResult_OK)
                return engine->runResult;
        }
        else
        {
            /* Ensure the parameter has not already been assigned. */
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

            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, ns, argumentSymbol, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
        }
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    /* Assign default values to remaining parameters. */
    iterationCount = 0;
    for (parameterResult = AspSequenceNext(engine, parameterList, 0, true);
         iterationCount < engine->cycleDetectionLimit &&
         parameterResult.element != 0;
         iterationCount++,
         parameterResult = AspSequenceNext
            (engine, parameterList, parameterResult.element, true))
    {
        AspDataEntry *parameter = parameterResult.value;
        int32_t parameterSymbol = AspDataGetParameterSymbol(parameter);

        if (AspDataGetParameterIsTupleGroup(parameter))
        {
            /* Create the tuple group if not already created. */
            if (tupleGroup == 0)
            {
                tupleGroup = AspAllocEntry(engine, DataType_Tuple);
                if (tupleGroup == 0)
                    return AspRunResult_OutOfDataMemory;
                AspTreeResult insertResult = AspTreeTryInsertBySymbol
                    (engine, ns, parameterSymbol, tupleGroup);
                if (insertResult.result != AspRunResult_OK)
                    return insertResult.result;
                AspUnref(engine, tupleGroup);
                if (engine->runResult != AspRunResult_OK)
                    return engine->runResult;
            }
        }
        else
        {
            /* Check if the parameter has been assigned a value. */
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

            /* Assign the default value to the parameter. */
            AspDataEntry *value = AspValueEntry
                (engine, AspDataGetParameterDefaultIndex(parameter));
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, ns, parameterSymbol, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
        }
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

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
    (AspEngine *engine, AspDataEntry *ns, int32_t symbol, bool dictionary)
{
    AspParameterResult result = {AspRunResult_OK, 0};

    AspTreeResult findResult = AspFindSymbol(engine, ns, symbol);
    result.result = findResult.result;
    result.value = findResult.value;
    if (result.result != AspRunResult_OK)
        return result;

    /* Ensure the parameter value is a tuple or dictionary, as applicable. */
    if (result.value == 0 ||
        AspDataGetType(result.value) !=
        (dictionary ? DataType_Dictionary : DataType_Tuple))
    {
        #ifdef ASP_DEBUG
        puts("Group parameter is not the expected type");
        #endif
        result.result = AspRunResult_InternalError;
        return result;
    }

    return result;
}
