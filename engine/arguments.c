/*
 * Asp engine arguments implementation.
 */

#include "asp-priv.h"
#include "symbols.h"
#include "sequence.h"
#include "tree.h"
#include "data.h"
#include <string.h>
#include <ctype.h>

static AspRunResult InitializeArguments
    (AspEngine *, AspDataEntry **arguments, AspDataEntry **argument0);
static AspRunResult ClearArguments(AspEngine *, AspDataEntry *arguments);
static AspSequenceResult AppendArgument(AspEngine *, AspDataEntry *arguments);

AspRunResult AspInitializeArguments(AspEngine *engine)
{
    return InitializeArguments(engine, 0, 0);
}

AspRunResult AspSetArguments(AspEngine *engine, const char * const *args)
{
    /* Initialize. */
    AspDataEntry *arguments, *argument0;
    AspRunResult initializeResult = InitializeArguments
        (engine, &arguments, &argument0);
    if (initializeResult != AspRunResult_OK)
        return initializeResult;

    /* Exit now if arguments are just being cleared. */
    if (args == 0)
        return AspRunResult_OK;

    /* Process the array of arguments. */
    for (const char * const *pp = args; *pp != 0; pp++)
    {
        const char *arg = *pp;

        /* In the first argument, separate the arguments with a space. */
        if (pp != args)
        {
            static char space = ' ';
            AspRunResult appendResult = AspStringAppendBuffer
                (engine, argument0, &space, 1);
            if (appendResult != AspRunResult_OK)
                return appendResult;
        }

        /* Append the characters of the argument to the first argument. */
        for (const char *p = arg; *p != '\0'; p++)
        {
            char c = *p;

            /* Append an espace character prior to special characters. */
            if (c == '\\' || c == '\'' || c == '"' || isspace(c))
            {
                static char escapeChar = '\\';
                AspRunResult appendResult = AspStringAppendBuffer
                    (engine, argument0, &escapeChar, 1);
                if (appendResult != AspRunResult_OK)
                    return appendResult;
            }

            /* Append the character. */
            AspRunResult appendResult = AspStringAppendBuffer
                (engine, argument0, &c, 1);
            if (appendResult != AspRunResult_OK)
                return appendResult;
        }

        /* Append the argument to the argument list. */
        AspSequenceResult appendArgumentResult = AppendArgument
            (engine, arguments);
        if (appendArgumentResult.result != AspRunResult_OK)
            return appendArgumentResult.result;
        AspDataEntry *argument = appendArgumentResult.value;
        AspRunResult appendResult = AspStringAppendBuffer
            (engine, argument, arg, strlen(arg));
        if (appendResult != AspRunResult_OK)
            return appendResult;
    }

    return AspRunResult_OK;
}

AspRunResult AspSetArgumentsString(AspEngine *engine, const char *s)
{
    /* Initialize. */
    AspDataEntry *arguments, *argument0;
    AspRunResult initializeResult = InitializeArguments
        (engine, &arguments, &argument0);
    if (initializeResult != AspRunResult_OK)
        return initializeResult;

    /* Exit now if arguments are just being cleared. */
    if (s == 0)
        return AspRunResult_OK;

    /* Process the arguments string. */
    bool inArgument = false, inString = false, escape = false;
    char c, quote;
    unsigned argumentIndex = AspDataGetSequenceCount(arguments);
    AspDataEntry *argument = 0;
    while (c = *s++)
    {
        /* Add the character to the first argument. */
        AspRunResult appendResult = AspStringAppendBuffer
            (engine, argument0, &c, 1);
        if (appendResult != AspRunResult_OK)
            return appendResult;

        /* Process special characters. */
        if (!escape)
        {
            if (!inString)
            {
                if (c == '\\')
                {
                    inArgument = true;
                    escape = true;
                    continue;
                }
                else if (c == '\'' || c == '"')
                {
                    quote = c;
                    inArgument = true;
                    inString = true;
                    continue;
                }
                else if (isspace(c))
                {
                    if (inArgument)
                        argumentIndex++;
                    inArgument = false;
                    continue;
                }
            }
            else
            {
                if (quote == '"' && c == '\\')
                {
                    escape = true;
                    continue;
                }
                else if (c == quote)
                {
                    inString = false;
                    continue;
                }
            }

            inArgument = true;
        }

        escape = false;

        /* Append new argument(s) if applicable. */
        while (AspDataGetSequenceCount(arguments) <= argumentIndex)
        {
            AspSequenceResult appendArgumentResult = AppendArgument
                (engine, arguments);
            if (appendArgumentResult.result != AspRunResult_OK)
                return appendArgumentResult.result;
            argument = appendArgumentResult.value;
        }

        /* Add the character to the current argument. */
        appendResult = AspStringAppendBuffer
            (engine, argument, &c, 1);
        if (appendResult != AspRunResult_OK)
            return appendResult;
    }

    /* Ensure the arguments string ended in an acceptable state. */
    if (inString || escape)
    {
        AspRunResult clearResult = ClearArguments(engine, arguments);
        if (clearResult != AspRunResult_OK)
            return clearResult;
        return AspRunResult_InitializationError;
    }

    /* Append any trailing empty arguments. */
    if (!inArgument)
        argumentIndex--;
    while (AspDataGetSequenceCount(arguments) <= argumentIndex)
    {
        AspSequenceResult appendResult = AppendArgument(engine, arguments);
        if (appendResult.result != AspRunResult_OK)
            return appendResult.result;
    }

    return AspRunResult_OK;
}

static AspRunResult InitializeArguments
    (AspEngine *engine, AspDataEntry **arguments, AspDataEntry **argument0)
{
    /* Ensure we're in the right state. */
    if ((arguments != 0 || argument0 != 0) &&
        engine->state != AspEngineState_Ready)
        return AspRunResult_InvalidState;

    /* Locate the arguments tuple. */
    AspTreeResult argumentsResult = AspFindSymbol
        (engine, engine->systemNamespace, AspSystemArgumentsSymbol);
    if (argumentsResult.result != AspRunResult_OK)
        return argumentsResult.result;
    if (AspDataGetType(argumentsResult.value) != DataType_Tuple)
        return AspRunResult_UnexpectedType;
    if (arguments != 0)
        *arguments = argumentsResult.value;

    /* Clear any previously defined arguments. */
    AspRunResult clearResult = ClearArguments
        (engine, argumentsResult.value);
    if (clearResult != AspRunResult_OK)
        return clearResult;

    /* Create the first argument which will contain the entire original
       arguments string. */
    AspSequenceResult appendResult = AppendArgument
        (engine, argumentsResult.value);
    if (appendResult.result != AspRunResult_OK)
        return appendResult.result;
    if (argument0)
        *argument0 = appendResult.value;

    return AspRunResult_OK;
}

static AspRunResult ClearArguments
    (AspEngine *engine, AspDataEntry *arguments)
{
    uint32_t iterationCount = 0;
    for (;
         iterationCount < engine->cycleDetectionLimit &&
         AspDataGetSequenceCount(arguments) != 0;
         iterationCount++)
    {
        if (!AspSequenceErase(engine, arguments, 0, true))
            return AspRunResult_InternalError;
    }
    if (iterationCount >= engine->cycleDetectionLimit)
        return AspRunResult_CycleDetected;

    return AspRunResult_OK;
}

static AspSequenceResult AppendArgument
    (AspEngine *engine, AspDataEntry *arguments)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};
    AspDataEntry *argument = AspAllocEntry(engine, DataType_String);
    if (argument == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    AspSequenceResult appendResult = AspSequenceAppend
        (engine, arguments, argument);
    if (appendResult.result == AspRunResult_OK)
    {
        AspUnref(engine, argument);
        if (engine->runResult != AspRunResult_OK)
        {
            result.result = engine->runResult;
            return result;
        }
    }
    return appendResult;
}
