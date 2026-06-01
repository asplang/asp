/*
 * Asp engine control implementation.
 */

#include "asp-priv.h"
#include "code.h"
#include "data.h"
#include "sequence.h"
#include "tree.h"
#include "arguments.h"
#include "function.h"
#include "reserved.h"
#include "appspec.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#if !defined ASP_ENGINE_VERSION_MAJOR || \
    !defined ASP_ENGINE_VERSION_MINOR
#error ASP_ENGINE_VERSION_* macros undefined
#endif

static void ProcessCodeHeader(AspEngine *);
static AspRunResult ResetData(AspEngine *);
static AspRunResult InitializeAppDefinitions(AspEngine *);
static AspRunResult LoadValue
    (AspEngine *, unsigned specSize, unsigned *specIndex, AspDataEntry **);
static AspRunResult LoadUnsignedInteger
    (const uint8_t *, unsigned specSize, unsigned *specIndex, uint32_t *);
static AspRunResult LoadSignedInteger
    (const uint8_t *, unsigned specSize, unsigned *specIndex, int32_t *);

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
    if ((codeSize != 0 && codeSize < HeaderSize) || codeSize > MaxCodeSize ||
        data == 0)
        return AspRunResult_InitializationError;

    engine->context = context;
    engine->floatConverter = floatConverter;
    engine->featureBits = 0;
    engine->state = AspEngineState_Reset;
    engine->codeArea = code;
    engine->maxCodeSize = codeSize;
    engine->cachedCodePageCount = 0;
    engine->codePageSize = 0;
    engine->cachedCodePages = 0;
    engine->codeReader = 0;
    engine->data = data;
    engine->maxDataSize = dataSize;
    engine->dataEndIndex = dataSize / AspDataEntrySize();
    engine->cycleDetectionLimit = (uint32_t)(engine->dataEndIndex / 2);
    engine->appSpec = appSpec;
    engine->inApp = false;

    #ifdef ASP_DEBUG
    engine->traceFile = stdout;
    #endif

    return AspReset(engine);
}

AspRunResult AspSetCodePaging
    (AspEngine *engine, uint8_t pageCount, size_t pageSize,
     AspCodeReader reader)
{
    if (engine->inApp || engine->state != AspEngineState_Reset)
        return AspRunResult_InvalidState;

    if (pageCount != 0 && (pageSize < HeaderSize || reader == 0))
        return AspRunResult_ValueOutOfRange;
    if (engine->state != AspEngineState_Reset)
        return AspRunResult_InvalidState;
    if (engine->codeArea == 0)
        return AspRunResult_InitializationError;

    if (pageSize == 0)
        pageCount = 0;
    size_t requiredSize = pageCount * pageSize;
    if (requiredSize > engine->maxCodeSize)
        return AspRunResult_InitializationError;
    size_t pageEntriesSize = pageCount * sizeof(AspCodePageEntry);
    if (pageEntriesSize >= engine->maxDataSize)
        return AspRunResult_OutOfDataMemory;

    engine->dataEndIndex =
        (engine->maxDataSize - pageEntriesSize) / AspDataEntrySize();
    engine->cachedCodePageCount = pageCount;
    engine->codePageSize = pageSize;
    engine->codeReader = reader;
    engine->cachedCodePages = (AspCodePageEntry *)(pageCount == 0 ? 0 :
        (uint8_t *)engine->data + engine->maxDataSize - pageEntriesSize);

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
             engine->state != AspEngineState_LoadingCode ||
             engine->code == 0)
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

    engine->codeEndKnown = true;
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
    engine->headerIndex = HeaderSize;
    engine->code = (uint8_t *)code;
    ProcessCodeHeader(engine);
    if (engine->loadResult != AspAddCodeResult_OK)
        return engine->loadResult;

    engine->code += HeaderSize;
    engine->codeEndIndex = codeSize - HeaderSize;
    engine->codeEndKnown = true;
    engine->state = AspEngineState_LoadingCode;
    return AspSeal(engine);
}

AspAddCodeResult AspPageCode(AspEngine *engine, void *id)
{
    if (engine->state != AspEngineState_Reset ||
        engine->codeArea == 0 || engine->cachedCodePageCount == 0)
        return AspAddCodeResult_InvalidState;

    /* Load the first page, which contains the header, and then ensure the
       header is valid. */
    engine->pagedCodeId = id;
    engine->headerIndex = HeaderSize;
    AspRunResult pageResult = AspLoadCodePage(engine, 0);
    if (pageResult != AspRunResult_OK)
        return AspAddCodeResult_InvalidFormat;
    engine->code = engine->codeArea;
    ProcessCodeHeader(engine);
    if (engine->loadResult != AspAddCodeResult_OK)
        return engine->loadResult;

    engine->code = 0;
    engine->state = AspEngineState_Ready;
    engine->runResult = AspRunResult_OK;
    return engine->loadResult = AspAddCodeResult_OK;
}

AspRunResult AspReset(AspEngine *engine)
{
    if (engine->inApp)
        return AspRunResult_InvalidState;

    engine->headerIndex = 0;
    engine->loadResult = AspAddCodeResult_OK;
    engine->runResult = AspRunResult_OK;
    memset(engine->version, 0, sizeof engine->version);
    if (engine->codeArea != 0)
        memset(engine->codeArea, 0, engine->maxCodeSize);
    engine->code = engine->codeArea;
    engine->codeEndIndex = 0;
    engine->pc = engine->instructionAddress = 0;
    engine->cachedCodePageIndex = 0;
    engine->codeEndKnown = false;
    engine->pagedCodeId = 0;
    engine->codePageReadCount = 0;
    if (engine->cachedCodePages != 0)
    {
        for (size_t i = 0; i < engine->cachedCodePageCount; i++)
        {
            AspCodePageEntry *entry = engine->cachedCodePages + i;
            entry->index = 0;
            entry->age = -1;
        }
    }
    engine->again = false;
    engine->callFromApp = false;
    engine->callReturning = false;
    engine->argumentList = 0;
    engine->appFunction = 0;
    engine->appFunctionNamespace = 0;
    engine->appFunctionReturnValue = 0;
    engine->nextSymbol = -1;

    AspRunResult result = ResetData(engine);
    if (result != AspRunResult_OK)
        return result;

    engine->state = AspEngineState_Reset;
    return AspRunResult_OK;
}

AspRunResult AspSetCycleDetectionLimit(AspEngine *engine, uint32_t limit)
{
    engine->cycleDetectionLimit = limit;
    return AspRunResult_OK;
}

uint32_t AspGetCycleDetectionLimit(const AspEngine *engine)
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

    engine->runResult = AspRunResult_OK;
    engine->pc = engine->instructionAddress = 0;
    engine->codePageReadCount = 0;
    engine->again = false;
    engine->callFromApp = false;
    engine->callReturning = false;
    engine->argumentList = 0;
    engine->appFunction = 0;
    engine->appFunctionNamespace = 0;
    engine->appFunctionReturnValue = 0;
    engine->nextSymbol = -1;

    AspRunResult result = ResetData(engine);
    if (result != AspRunResult_OK)
        return result;

    engine->state = AspEngineState_Ready;
    return AspRunResult_OK;
}

static void ProcessCodeHeader(AspEngine *engine)
{
    /* Ensure the application specification has been specified. */
    if (engine->appSpec == 0)
    {
        engine->loadResult = AspAddCodeResult_InvalidState;
        return;
    }

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
    /* Destroy application objects if applicable. */
    bool isRunning =
        engine->state == AspEngineState_Running ||
        engine->state == AspEngineState_RunError ||
        engine->state == AspEngineState_Ended;
    if (isRunning)
    {
        AspDataEntry *entry = engine->data;
        for (unsigned i = 0; i < engine->dataEndIndex; i++, entry++)
        {
            uint8_t type = AspDataGetType(entry);
            if (type == DataType_AppIntegerObject ||
                type == DataType_AppPointerObject)
            {
                AspDataSetUseCount(entry, 1U);
                AspUnref(engine, entry);
            }
        }
    }

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
    engine->ellipsisSingleton = 0;
    engine->falseSingleton = 0;
    engine->trueSingleton = 0;

    #ifdef ASP_FEATURE_CLASS

    /* Initialize the base of all non-derived classes. */
    engine->objectClass = 0;

    #endif

    /* Initialize stack. */
    engine->stackTop = 0;
    engine->stackCount = 0;

    /* Create empty modules collection. */
    engine->modules = AspAllocEntry(engine, DataType_Namespace);
    if (engine->modules == 0)
        return AspRunResult_OutOfDataMemory;

    /* Create the system module. This is where top-level application
       definitions go. */
    engine->systemNamespace = AspAllocEntry(engine, DataType_Namespace);
    if (engine->systemNamespace == 0)
        return AspRunResult_OutOfDataMemory;
    engine->systemModule = AspAllocEntry(engine, DataType_Module);
    if (engine->systemModule == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetModuleIsApp(engine->systemModule, true);
    AspDataSetModuleSymbol(engine->systemModule, 0);
    AspDataSetModuleNamespaceIndex
        (engine->systemModule, AspIndex(engine, engine->systemNamespace));
    AspDataSetModuleIsLoaded(engine->systemModule, true);
    AspTreeResult addSystemModuleResult = AspTreeTryInsertBySymbol
        (engine, engine->modules,
         AspReservedSymbol_SystemModule, engine->systemModule);
    if (addSystemModuleResult.result != AspRunResult_OK)
        return addSystemModuleResult.result;
    AspUnref(engine, engine->systemModule);
    engine->module = engine->systemModule;

    /* Add an empty arguments tuple to the system module. */
    AspDataEntry *arguments = AspAllocEntry(engine, DataType_Tuple);
    if (arguments == 0)
        return AspRunResult_OutOfDataMemory;
    AspTreeResult addArgumentsResult = AspTreeTryInsertBySymbol
        (engine, engine->systemNamespace,
         AspReservedSymbol_SystemArguments, arguments);
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

    /* Determine the version of the specification string and ensure it's
       valid. */
    unsigned specIndex = 0;
    const uint8_t *spec = (const uint8_t *)engine->appSpec->spec;
    unsigned specSize = engine->appSpec->specSize;
    uint8_t version = 0;
    if (specSize >= 3 && spec[0] == 0xFF && spec[1] == 0xFF)
    {
        specIndex += 2;
        version = spec[specIndex++];
    }
    if (version > 2u)
        return AspRunResult_InitializationError;

    /* Enable applicable features. */
    if (version >= 2u)
        engine->featureBits = spec[specIndex++];

    /* Create application modules if applicable. */
    int32_t appModuleCount = 0;
    if (version >= 1u)
    {
        AspRunResult result = LoadSignedInteger
            (spec, specSize, &specIndex, &appModuleCount);
        if (result != AspRunResult_OK)
            return result;
        if (appModuleCount < 0)
            return AspRunResult_InitializationError;

        /* Create the application modules. */
        for (int32_t appModuleSymbol = -1;
             appModuleSymbol >= -appModuleCount;
             appModuleSymbol--)
        {
            /* Create a namespace for the application module. */
            AspDataEntry *appNamespace = AspAllocEntry
                (engine, DataType_Namespace);
            if (appNamespace == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataEntry *appModule = AspAllocEntry
                (engine, DataType_Module);
            if (appModule == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetModuleIsApp(appModule, true);
            AspDataSetModuleSymbol(appModule, appModuleSymbol);
            AspDataSetModuleNamespaceIndex
                (appModule, AspIndex(engine, appNamespace));
            AspDataSetModuleIsLoaded(appModule, true);

            /* Insert the application module into the modules collection by its
               temporary symbol. This entry will be removed after all the
               application module imports have been processed. */
            AspTreeResult addAppModuleResult = AspTreeTryInsertBySymbol
                (engine, engine->modules, appModuleSymbol, appModule);
            if (addAppModuleResult.result != AspRunResult_OK)
                return addAppModuleResult.result;
            AspUnref(engine, appModule);
        }
    }

    /* Create definitions for application variables, functions, and application
       module imports. Note that the first few symbols are reserved. */
    int32_t nextAppModuleId = 0;
    AspDataEntry *currentAppModule = engine->module;
    AspDataEntry *currentAppNamespace = engine->systemNamespace;
    for (int32_t nextSymbol = AspReservedSymbol_End;
         nextSymbol <= AspSignedWordMax; nextSymbol++)
    {
        if (specIndex >= specSize)
            break;

        /* Read the entry prefix. */
        uint8_t prefix = spec[specIndex++];

        /* Determine the entry's symbol. */
        int32_t symbol = nextSymbol;
        if (version >= 1u)
        {
            /* Ensure a symbol prefix does not appear in a version 1 or greater
               spec. */
            if (prefix == AppSpecPrefix_Symbol)
                return AspRunResult_InitializationError;

            /* Read a symbol for all entry types except for imports and
               modules, whose symbols are determined sequentially. */
            if (prefix != AppSpecPrefix_Import &&
                prefix != AppSpecPrefix_Module)
            {
                AspRunResult result = LoadSignedInteger
                    (spec, specSize, &specIndex, &symbol);
                if (result != AspRunResult_OK)
                    return result;
            }
        }

        /* Read the rest of the entry. */
        if (prefix == AppSpecPrefix_Variable)
        {
            /* Create the variable's value. */
            AspDataEntry *value = 0;
            AspRunResult loadValueResult = LoadValue
                (engine, specSize, &specIndex, &value);
            if (loadValueResult != AspRunResult_OK)
                return loadValueResult;

            /* Insert the variable into the current namespace. */
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, currentAppNamespace, symbol, value);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            if (!insertResult.inserted)
                return AspRunResult_InitializationError;
            AspUnref(engine, value);
        }
        else if (version >= 1u && prefix == AppSpecPrefix_Module)
        {
            /* Locate the next application module. */
            AspTreeResult findAppModuleResult = AspFindSymbol
                (engine, engine->modules, --nextAppModuleId);
            if (findAppModuleResult.result != AspRunResult_OK)
                return findAppModuleResult.result;
            if (findAppModuleResult.node == 0)
                return AspRunResult_InitializationError;
            currentAppModule = findAppModuleResult.value;
            if (AspDataGetType(currentAppModule) != DataType_Module)
                return AspRunResult_InitializationError;

            /* Switch to the module's namespace for inserting new items. */
            currentAppNamespace = AspEntry
                (engine, AspDataGetModuleNamespaceIndex(currentAppModule));
        }
        else if (version >= 1u && prefix == AppSpecPrefix_Import)
        {
            /* Read the symbol of the application module to import. */
            int32_t appModuleSymbol;
            AspRunResult result = LoadSignedInteger
                (spec, specSize, &specIndex, &appModuleSymbol);
            if (result != AspRunResult_OK)
                return result;

            /* Locate the application module. */
            AspTreeResult findAppModuleResult = AspFindSymbol
                (engine, engine->modules, appModuleSymbol);
            if (findAppModuleResult.result != AspRunResult_OK)
                return findAppModuleResult.result;
            if (findAppModuleResult.node == 0)
                return AspRunResult_InitializationError;
            AspDataEntry *appModule = findAppModuleResult.value;
            if (AspDataGetType(appModule) != DataType_Module)
                return AspRunResult_InitializationError;

            /* Insert the application module into the modules collection by its
               import symbol. */
            AspTreeResult moduleInsertResult = AspTreeTryInsertBySymbol
                (engine, engine->modules, symbol, appModule);
            if (moduleInsertResult.result != AspRunResult_OK)
                return moduleInsertResult.result;
            if (!moduleInsertResult.inserted)
                return AspRunResult_InitializationError;
        }
        else if (version >= 1u && prefix == AppSpecPrefix_Function ||
                 prefix != AppSpecPrefix_Symbol)
        {
            /* Determine the number of parameters for the function. */
            uint32_t parameterCount;
            if (version == 0 || prefix != AppSpecPrefix_Function)
                parameterCount = prefix;
            else
            {
                AspRunResult result = LoadUnsignedInteger
                    (spec, specSize, &specIndex, &parameterCount);
                if (result != AspRunResult_OK)
                    return result;
            }

            /* Create the function's parameter list. */
            AspDataEntry *parameters = AspAllocEntry
                (engine, DataType_ParameterList);
            if (parameters == 0)
                return AspRunResult_OutOfDataMemory;
            for (uint32_t p = 0; p < parameterCount; p++)
            {
                uint32_t parameterSpec;
                AspRunResult result = LoadUnsignedInteger
                    (spec, specSize, &specIndex, &parameterSpec);
                if (result != AspRunResult_OK)
                    return result;
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
                        (engine, specSize, &specIndex, &defaultValue);
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

            /* Create the function. */
            AspDataEntry *function = AspAllocEntry(engine, DataType_Function);
            if (function == 0)
                return AspRunResult_OutOfDataMemory;
            AspDataSetFunctionSymbol(function, symbol);
            AspDataSetFunctionIsApp(function, true);
            AspRef(engine, currentAppModule);
            AspDataSetFunctionModuleIndex
                (function, AspIndex(engine, currentAppModule));
            AspDataSetFunctionParametersIndex
                (function, AspIndex(engine, parameters));

            /* Insert the function into the current namespace. */
            AspTreeResult insertResult = AspTreeTryInsertBySymbol
                (engine, currentAppNamespace, symbol, function);
            if (insertResult.result != AspRunResult_OK)
                return insertResult.result;
            if (!insertResult.inserted)
                return AspRunResult_InitializationError;
            AspUnref(engine, function);
        }
    }

    /* Drop temporary references to any application modules. */
    for (int32_t appModuleSymbol = -1;
         appModuleSymbol >= -appModuleCount;
         appModuleSymbol--)
    {
        /* Locate the application module by its temporary symbol. */
        AspTreeResult findAppModuleResult = AspFindSymbol
            (engine, engine->modules, appModuleSymbol);
        if (findAppModuleResult.result != AspRunResult_OK)
            return findAppModuleResult.result;
        if (findAppModuleResult.node == 0)
            return AspRunResult_InitializationError;

        /* Remove the temporary application module from the modules
           collection. */
        AspRunResult eraseResult = AspTreeEraseNode
            (engine, engine->modules, findAppModuleResult.node,
             true, true);
        if (eraseResult != AspRunResult_OK)
            return eraseResult;
    }

    /* Ensure we read the application spec correctly. */
    if (specIndex != specSize)
        return AspRunResult_InitializationError;

    #ifdef ASP_FEATURE_CLASS

    if (AspIsFeature(engine, AspFeatureBit_Class))
    {
        /* Create the common base of all non-derived classes. */
        engine->objectClass = AspAllocEntry(engine, DataType_Class);
        if (engine->objectClass == 0)
            return AspRunResult_OutOfDataMemory;
        AspDataEntry *objectClassNamespace = AspAllocEntry
            (engine, DataType_Namespace);
        if (objectClassNamespace == 0)
            return AspRunResult_OutOfDataMemory;
        AspDataSetClassNamespaceIndex
            (engine->objectClass, AspIndex(engine, objectClassNamespace));

        /* Locate the common base class initialization function in the system
           module. */
        AspTreeResult initFunctionFindResult = AspFindSymbol
            (engine, engine->systemNamespace,
             AspReservedSymbol_ClassInitialize);
        if (initFunctionFindResult.result != AspRunResult_OK)
            return initFunctionFindResult.result;
        if (initFunctionFindResult.value == 0)
            return AspRunResult_NameNotFound;
        if (AspDataGetType(initFunctionFindResult.value) != DataType_Function)
            return AspRunResult_UnexpectedType;

        /* Insert the initialization function into the common base class. */
        AspTreeResult insertResult = AspTreeTryInsertBySymbol
            (engine, objectClassNamespace,
             AspReservedSymbol_ClassInitialize, initFunctionFindResult.value);
        if (insertResult.result != AspRunResult_OK)
            return insertResult.result;
    }

    #endif

    return AspRunResult_OK;
}

static AspRunResult LoadValue
    (AspEngine *engine, unsigned specSize, unsigned *specIndex,
     AspDataEntry **valueEntry)
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
            if (*specIndex + 1 > specSize)
                return AspRunResult_InitializationError;
            uint8_t value = spec[(*specIndex)++];
            *valueEntry = AspNewBoolean(engine, value != 0);
            break;
        }

        case AppSpecValueType_Integer:
        {
            int32_t value;
            AspRunResult result = LoadSignedInteger
                (spec, specSize, specIndex, &value);
            if (result != AspRunResult_OK)
                return result;
            *valueEntry = AspNewInteger(engine, value);
            break;
        }

        case AppSpecValueType_Float:
        {
            if (*specIndex + 4 > specSize)
                return AspRunResult_InitializationError;
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
            if (*specIndex + 4 > specSize)
                return AspRunResult_InitializationError;
            uint32_t valueSize = 0;
            for (unsigned i = 0; i < 4; i++)
            {
                valueSize <<= 8;
                valueSize |= spec[(*specIndex)++];
            }
            if (*specIndex + valueSize > specSize)
                return AspRunResult_InitializationError;
            *valueEntry = AspNewString
                (engine, (const char *)(spec + *specIndex), valueSize);
            *specIndex += valueSize;
            break;
        }
    }

    return
        *valueEntry == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

static AspRunResult LoadUnsignedInteger
    (const uint8_t *spec, unsigned specSize, unsigned *specIndex,
     uint32_t *value)
{
    if (*specIndex + 4 > specSize)
        return AspRunResult_InitializationError;

    *value = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        *value <<= 8;
        *value |= spec[(*specIndex)++];
    }
    return AspRunResult_OK;
}

static AspRunResult LoadSignedInteger
    (const uint8_t *spec, unsigned specSize, unsigned *specIndex,
     int32_t *value)
{
    uint32_t uValue;
    AspRunResult result = LoadUnsignedInteger
        (spec, specSize, specIndex, &uValue);
    if (result != AspRunResult_OK)
        return result;

    *value = *(int32_t *)&uValue;
    return AspRunResult_OK;
}

AspRunResult AspGetRunResult(const AspEngine *engine)
{
    return engine->runResult;
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
    return (size_t)engine->pc;
}

size_t AspFreeCount(const AspEngine *engine)
{
    return engine->freeCount;
}

size_t AspLowFreeCount(const AspEngine *engine)
{
    return engine->lowFreeCount;
}

size_t AspCodePageReadCount(AspEngine *engine, bool reset)
{
    size_t count = engine->codePageReadCount;
    if (reset)
        engine->codePageReadCount = 0;
    return count;
}

uint32_t AspUseCount(const AspDataEntry *entry)
{
    return entry == 0 ? 0 : AspDataGetUseCount(entry);
}
