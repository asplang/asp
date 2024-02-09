/*
 * Asp engine control implementation.
 */

#include "asp-priv.h"
#include "data.h"
#include "sequence.h"
#include "tree.h"
#include "arguments.h"
#include "function.h"
#include "symbols.h"
#include "appspec.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#ifdef ASP_DEBUG
#include "debug.h"
#include <stdio.h>
#include <ctype.h>
#endif

#if !defined ASP_ENGINE_VERSION_MAJOR || \
    !defined ASP_ENGINE_VERSION_MINOR
#error ASP_ENGINE_VERSION_* macros undefined
#endif

static void ProcessCodeHeader(AspEngine *);
static AspRunResult ResetData(AspEngine *);
static AspRunResult InitializeAppDefinitions(AspEngine *);
static AspRunResult LoadValue
    (AspEngine *, unsigned *specIndex, AspDataEntry **);

static const uint8_t HeaderSize = 12;
static const size_t MaxCodeSize = 1 << AspWordBitSize;
static const uint32_t ParameterSpecMask = ((1U << AspWordBitSize) - 1U);

AspRunResult AspInitialize
    (AspEngine *engine,
     void *code, size_t codeSize,
     void *data, size_t dataSize,
     const AspAppSpec *appSpec, void *context)
{
    return AspInitializeEx
        (engine, code, codeSize, data, dataSize,
         appSpec, context, 0);
}

AspRunResult AspInitializeEx
    (AspEngine *engine,
     void *code, size_t codeSize,
     void *data, size_t dataSize,
     const AspAppSpec *appSpec, void *context,
     AspFloatConverter floatConverter)
{
    if (codeSize > MaxCodeSize)
        return AspRunResult_InitializationError;

    engine->context = context;
    engine->floatConverter = floatConverter;
    engine->codeArea = code;
    engine->maxCodeSize = codeSize;
    engine->data = data;
    engine->dataEndIndex = dataSize / AspDataEntrySize();
    engine->cycleDetectionLimit = (uint32_t)(engine->dataEndIndex / 2);
    engine->appSpec = appSpec;
    engine->inApp = false;

    return AspReset(engine);
}

void AspCodeVersion
    (const AspEngine *engine, uint8_t version[sizeof engine->version])
{
    memcpy(version, engine->version, sizeof engine->version);
}

size_t AspMaxCodeSize(const AspEngine *engine)
{
    return engine->maxCodeSize;
}

size_t AspMaxDataSize(const AspEngine *engine)
{
    return engine->dataEndIndex;
}

AspAddCodeResult AspAddCode
    (AspEngine *engine, const void *code, size_t codeSize)
{
    if (engine->state == AspEngineState_LoadError)
        return engine->loadResult;
    else if (engine->state == AspEngineState_Reset)
    {
        engine->state = AspEngineState_LoadingHeader;
        engine->headerIndex = 0;
    }
    else if (engine->state != AspEngineState_LoadingHeader &&
             engine->state != AspEngineState_LoadingCode)
        return AspAddCodeResult_InvalidState;

    const uint8_t *codePtr = (const uint8_t *)code;
    if (engine->state == AspEngineState_LoadingHeader)
    {
        while (engine->headerIndex < HeaderSize && codeSize > 0)
        {
            engine->code[engine->headerIndex++] = *codePtr++;
            codeSize--;

            if (engine->headerIndex == HeaderSize)
            {
                /* Ensure the header is valid. */
                ProcessCodeHeader(engine);
                if (engine->loadResult != AspAddCodeResult_OK)
                    return engine->loadResult;

                engine->state = AspEngineState_LoadingCode;
                break;
            }
        }

        if (engine->state == AspEngineState_LoadingHeader ||
            engine->state == AspEngineState_LoadError)
            return engine->loadResult;
    }

    if (engine->state != AspEngineState_LoadingCode)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidState;
    }

    /* Ensure there's enough room to copy the code. */
    if (engine->codeEndIndex + codeSize > engine->maxCodeSize)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_OutOfCodeMemory;
    }

    memcpy(engine->code + engine->codeEndIndex, codePtr, codeSize);
    engine->codeEndIndex += codeSize;

    return engine->loadResult;
}

AspAddCodeResult AspSeal(AspEngine *engine)
{
    /* Ensure we got past loading the header. */
    if (engine->state != AspEngineState_LoadingCode)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidFormat;
        return engine->loadResult;
    }

    engine->state = AspEngineState_Ready;
    engine->runResult = AspRunResult_OK;
    return engine->loadResult;
}

AspAddCodeResult AspSealCode
    (AspEngine *engine, const void *code, size_t codeSize)
{
    if (engine->state == AspEngineState_LoadError)
        return engine->loadResult;
    else if (engine->state != AspEngineState_Reset)
        return AspAddCodeResult_InvalidState;

    /* Ensure the header is present and valid. */
    if (codeSize < HeaderSize)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidFormat;
        return engine->loadResult;
    }
    engine->pc = engine->code = (uint8_t *)code;
    ProcessCodeHeader(engine);
    if (engine->loadResult != AspAddCodeResult_OK)
        return engine->loadResult;

    engine->pc = engine->code += HeaderSize;
    engine->codeEndIndex = codeSize - HeaderSize;
    engine->state = AspEngineState_LoadingCode;
    return AspSeal(engine);
}

AspRunResult AspReset(AspEngine *engine)
{
    if (engine->inApp)
        return AspRunResult_InvalidState;

    engine->state = AspEngineState_Reset;
    engine->headerIndex = 0;
    engine->loadResult = AspAddCodeResult_OK;
    engine->again = false;
    engine->runResult = AspRunResult_OK;
    memset(engine->version, 0, sizeof engine->version);
    memset(engine->codeArea, 0, engine->maxCodeSize);
    engine->pc = engine->code = engine->codeArea;
    engine->codeEndIndex = 0;
    engine->appFunctionSymbol = 0;
    engine->appFunctionNamespace = 0;
    engine->appFunctionReturnValue = 0;

    return ResetData(engine);
}

AspRunResult AspSetCycleDetectionLimit(AspEngine *engine, uint32_t limit)
{
    engine->cycleDetectionLimit = limit;
    return AspRunResult_OK;
}

uint32_t AspGetCycleDetectionLimit(AspEngine *engine)
{
    return engine->cycleDetectionLimit;
}

AspRunResult AspRestart(AspEngine *engine)
{
    if (engine->inApp)
        return AspRunResult_InvalidState;

    if (engine->state != AspEngineState_Ready &&
        engine->state != AspEngineState_Running &&
        engine->state != AspEngineState_RunError &&
        engine->state != AspEngineState_Ended)
        return AspRunResult_InvalidState;

    engine->state = AspEngineState_Ready;
    engine->again = false;
    engine->runResult = AspRunResult_OK;
    engine->pc = engine->code;
    engine->appFunctionSymbol = 0;
    engine->appFunctionNamespace = 0;
    engine->appFunctionReturnValue = 0;

    return ResetData(engine);
}

static void ProcessCodeHeader(AspEngine *engine)
{
    /* Check the header signature. */
    if (memcmp(engine->code, "AspE", 4) != 0)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidFormat;
        return;
    }

    /* Check the version of the executable to ensure
       compatibility. */
    memcpy(engine->version, engine->code + 4, sizeof engine->version);
    if (engine->version[0] != ASP_ENGINE_VERSION_MAJOR ||
        engine->version[1] != ASP_ENGINE_VERSION_MINOR)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidVersion;
        return;
    }

    /* Check the application specification check value. */
    const uint8_t *checkValuePtr = engine->code + 8;
    uint32_t checkValue = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        checkValue <<= 8;
        checkValue |= *checkValuePtr++;
    }
    if (checkValue != engine->appSpec->checkValue)
    {
        engine->state = AspEngineState_LoadError;
        engine->loadResult = AspAddCodeResult_InvalidCheckValue;
        return;
    }
}

static AspRunResult ResetData(AspEngine *engine)
{
    /* Clear data storage, setting every element to a free entry. */
    AspClearData(engine);

    /* Allocate the None singleton. Note that this is the only time we expect
       a zero index returned from AspAlloc to be valid. Subsequently, a zero
       return value will indicate an allocation error. */
    if (engine->freeCount == 0)
        return AspRunResult_OutOfDataMemory;
    uint32_t noneSingletonIndex = AspAlloc(engine);
    AspRunResult assertResult = AspAssert
        (engine, noneSingletonIndex == 0);
    if (assertResult != AspRunResult_OK)
        return assertResult;
    engine->noneSingleton = engine->data;
    assertResult = AspAssert
        (engine, AspDataGetType(engine->noneSingleton) == DataType_None);
    if (assertResult != AspRunResult_OK)
        return assertResult;
    AspRef(engine, engine->noneSingleton);

    /* Initialize singletons for other commonly used objects. */
    engine->falseSingleton = 0;
    engine->trueSingleton = 0;

    /* Initialize stack. */
    engine->stackTop = 0;
    engine->stackCount = 0;

    /* Create empty modules collection. */
    engine->modules = AspAllocEntry(engine, DataType_Namespace);
    if (engine->modules == 0)
        return AspRunResult_OutOfDataMemory;

    /* Create the system module. This is where app function definitions go. */
    engine->systemNamespace = AspAllocEntry(engine, DataType_Namespace);
    if (engine->systemNamespace == 0)
        return AspRunResult_OutOfDataMemory;
    engine->systemModule = AspAllocEntry(engine, DataType_Module);
    if (engine->systemModule == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetModuleCodeAddress(engine->systemModule, 0);
    AspDataSetModuleNamespaceIndex
        (engine->systemModule, AspIndex(engine, engine->systemNamespace));
    AspDataSetModuleIsLoaded(engine->systemModule, true);
    AspTreeResult addSystemModuleResult = AspTreeTryInsertBySymbol
        (engine, engine->modules, AspSystemModuleSymbol, engine->systemModule);
    AspUnref(engine, engine->systemModule);
    if (addSystemModuleResult.result != AspRunResult_OK)
        return addSystemModuleResult.result;
    engine->module = engine->systemModule;

    /* Add an empty arguments tuple to the system module. */
    AspDataEntry *arguments = AspAllocEntry(engine, DataType_Tuple);
    if (arguments == 0)
        return AspRunResult_OutOfDataMemory;
    AspTreeResult addArgumentsResult = AspTreeTryInsertBySymbol
        (engine, engine->systemNamespace, AspSystemArgumentsSymbol, arguments);
    if (addArgumentsResult.result != AspRunResult_OK)
        return addArgumentsResult.result;
    AspUnref(engine, arguments);
    AspRunResult argumentsResult = AspInitializeArguments(engine);
    if (argumentsResult != AspRunResult_OK)
        return argumentsResult;

    /* Set local and global namespaces initially to the system namespace. */
    engine->localNamespace = engine->globalNamespace = engine->systemNamespace;

    /* Initialize application definitions. */
    AspRunResult appDefinitionsResult = InitializeAppDefinitions(engine);
    if (appDefinitionsResult != AspRunResult_OK)
        return appDefinitionsResult;

    return AspRunResult_OK;
}

static AspRunResult InitializeAppDefinitions(AspEngine *engine)
{
    if (engine->appSpec == 0)
    {
        #ifdef ASP_TEST
        return AspRunResult_OK;
        #else
        return AspRunResult_InitializationError;
        #endif
    }

    /* Create definitions for application variables and functions.
       Note that the first few symbols are reserved:
       0 - main module name
       1 - args */
    unsigned specIndex = 0;
    const uint8_t *spec = (const uint8_t *)engine->appSpec->spec;
    for (int32_t symbol = AspScriptSymbolBase;
         symbol <= AspSignedWordMax; symbol++)
    {
        if (specIndex >= engine->appSpec->specSize)
            break;

        uint8_t prefix = spec[specIndex++];
        if (prefix == AppSpecPrefix_Variable)
        {
            AspDataEntry *value = 0;
            AspRunResult loadValueResult = LoadValue
                (engine, &specIndex, &value);
            if (loadValueResult != AspRunResult_OK)
                return loadValueResult;

            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, engine->systemNamespace, symbol, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            if (!insertResult.inserted)
                return AspRunResult_InitializationError;
            AspUnref(engine, value);
        }
        else if (prefix != AppSpecPrefix_Symbol)
        {
            unsigned parameterCount = prefix;

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
                    parameterSpec |= spec[specIndex++];
                }
                uint32_t parameterSymbol =
                    (int32_t)(parameterSpec & ParameterSpecMask);
                uint8_t parameterType =
                    (int8_t)(parameterSpec >> AspWordBitSize);
                bool hasDefault =
                    parameterType == AppSpecParameterType_Defaulted;

                AspDataEntry *parameter = AspAllocEntry
                    (engine, DataType_Parameter);
                if (parameter == 0)
                    return AspRunResult_OutOfDataMemory;
                AspDataSetParameterSymbol(parameter, parameterSymbol);
                AspDataSetParameterHasDefault(parameter, hasDefault);
                AspDataSetParameterIsTupleGroup
                    (parameter,
                     parameterType == AppSpecParameterType_TupleGroup);
                AspDataSetParameterIsDictionaryGroup
                    (parameter,
                     parameterType == AppSpecParameterType_DictionaryGroup);

                if (hasDefault)
                {
                    AspDataEntry *defaultValue = 0;
                    AspRunResult loadValueResult = LoadValue
                        (engine, &specIndex, &defaultValue);
                    if (loadValueResult != AspRunResult_OK)
                        return loadValueResult;
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
            AspDataSetFunctionSymbol(function, symbol);
            AspDataSetFunctionIsApp(function, true);
            AspRef(engine, engine->module);
            AspDataSetFunctionModuleIndex
                (function, AspIndex(engine, engine->module));
            AspDataSetFunctionParametersIndex
                (function, AspIndex(engine, parameters));

            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, engine->systemNamespace, symbol, function);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            if (!insertResult.inserted)
                return AspRunResult_InitializationError;
            AspUnref(engine, function);
        }
    }

    /* Ensure we read the application spec correctly. */
    return
        specIndex != engine->appSpec->specSize ?
        AspRunResult_InitializationError : AspRunResult_OK;
}

static AspRunResult LoadValue
    (AspEngine *engine, unsigned *specIndex, AspDataEntry **valueEntry)
{
    const uint8_t *spec = (const uint8_t *)engine->appSpec->spec;
    uint32_t valueType = spec[(*specIndex)++];
    switch (valueType)
    {
        default:
            return AspRunResult_InitializationError;

        case AppSpecValueType_None:
            *valueEntry = AspNewNone(engine);
            break;

        case AppSpecValueType_Ellipsis:
            *valueEntry = AspNewEllipsis(engine);
            break;

        case AppSpecValueType_Boolean:
        {
            uint8_t value = spec[(*specIndex)++];
            *valueEntry = AspNewBoolean(engine, value != 0);
            break;
        }

        case AppSpecValueType_Integer:
        {
            uint32_t uValue = 0;
            for (unsigned i = 0; i < 4; i++)
            {
                uValue <<= 8;
                uValue |= spec[(*specIndex)++];
            }
            int32_t value = *(int32_t *)&uValue;
            *valueEntry = AspNewInteger(engine, value);
            break;
        }

        case AppSpecValueType_Float:
        {
            static const uint16_t word = 1;
            bool be = *(const char *)&word == 0;

            uint8_t data[8];
            for (unsigned i = 0; i < 8; i++)
                data[be ? i : 7 - i] = spec[(*specIndex)++];

            /* Convert IEEE 754 binary64 to the native format. */
            double value = engine->floatConverter != 0 ?
                engine->floatConverter(data) : *(double *)data;
            *valueEntry = AspNewFloat(engine, value);

            break;
        }

        case AppSpecValueType_String:
        {
            uint32_t valueSize = 0;
            for (unsigned i = 0; i < 4; i++)
            {
                valueSize <<= 8;
                valueSize |= spec[(*specIndex)++];
            }
            *valueEntry = AspNewString
                (engine, spec + *specIndex, valueSize);
            *specIndex += valueSize;
            break;
        }
    }

    return
        *valueEntry == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

bool AspIsReady(const AspEngine *engine)
{
    return engine->state == AspEngineState_Ready;
}

bool AspIsRunning(const AspEngine *engine)
{
    return engine->state == AspEngineState_Running;
}

bool AspIsRunnable(const AspEngine *engine)
{
    return
        engine->state == AspEngineState_Ready ||
        engine->state == AspEngineState_Running;
}

size_t AspProgramCounter(const AspEngine *engine)
{
    return (size_t)(engine->pc - engine->code);
}

size_t AspLowFreeCount(const AspEngine *engine)
{
    return engine->lowFreeCount;
}
