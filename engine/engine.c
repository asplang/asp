/*
 * Asp engine control implementation.
 */

#include "asp-priv.h"
#include "data.h"
#include "tree.h"
#include "function.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#ifdef ASP_DEBUG
#include "debug.h"
#include <stdio.h>
#include <ctype.h>
#endif

#define HEADER_SIZE 8
#define MAX_CODE_SIZE (1 << (AspWordBitSize))

static AspRunResult ResetData(AspEngine *);

AspRunResult AspInitialize
    (AspEngine *engine,
     void *code, size_t codeSize,
     void *data, size_t dataSize,
     AspAppSpec *appSpec, void *context)
{
    if (codeSize > MAX_CODE_SIZE)
        return AspRunResult_InitializationError;

    engine->context = context;
    engine->code = code;
    engine->maxCodeSize = codeSize;
    engine->data = data;
    engine->dataEndIndex = dataSize / AspDataEntrySize();
    engine->appSpec = appSpec;
    engine->inApp = false;

    return AspReset(engine);
}

AspAddCodeResult AspAddCode
    (AspEngine *engine, const char *code, size_t codeSize)
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

    if (engine->state == AspEngineState_LoadingHeader)
    {
        while (engine->headerIndex < HEADER_SIZE && codeSize > 0)
        {
            engine->code[engine->headerIndex++] = *code++;
            codeSize--;

            if (engine->headerIndex == HEADER_SIZE)
            {
                /* Check the header signature. */
                if (memcmp(engine->code, "AspE", 4) != 0)
                {
                    engine->state = AspEngineState_LoadError;
                    engine->loadResult = AspAddCodeResult_InvalidFormat;
                    return engine->loadResult;
                }

                /* Check the application specification check value. */
                const uint8_t *checkValuePtr = engine->code + 4;
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
                    return engine->loadResult;
                }

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

    memcpy(engine->code + engine->codeEndIndex, code, codeSize);
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

AspRunResult AspReset(AspEngine *engine)
{
    if (engine->inApp)
        return AspRunResult_InvalidState;

    engine->state = AspEngineState_Reset;
    engine->headerIndex = 0;
    engine->loadResult = AspAddCodeResult_OK;
    engine->again = false;
    engine->runResult = AspRunResult_OK;
    engine->pc = engine->code;
    engine->codeEndIndex = 0;
    memset(engine->code, 0, engine->maxCodeSize);
    engine->appFunctionSymbol = 0;
    engine->appFunctionNamespace = 0;
    engine->appFunctionReturnValue = 0;

    return ResetData(engine);
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

    ResetData(engine);
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

    /* Initialize stack. */
    engine->stackTop = 0;
    engine->stackCount = 0;

    /* Create empty modules collection. */
    engine->modules = AspAllocEntry(engine, DataType_Namespace);
    if (engine->modules == 0)
        return AspRunResult_OutOfDataMemory;

    /* Create system module. This is where app function definitions go. */
    engine->systemNamespace = AspAllocEntry(engine, DataType_Namespace);
    if (engine->systemNamespace == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataEntry *systemModule = AspAllocEntry(engine, DataType_Module);
    if (systemModule == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetModuleCodeAddress(systemModule, 0);
    AspDataSetModuleNamespaceIndex
        (systemModule, AspIndex(engine, engine->systemNamespace));
    AspDataSetModuleIsLoaded(systemModule, true);
    AspTreeResult addResult = AspTreeTryInsertBySymbol
        (engine, engine->modules, -1, systemModule);
    AspUnref(engine, systemModule);
    if (addResult.result != AspRunResult_OK)
        return addResult.result;
    engine->module = systemModule;

    /* Set local and global namespaces initially to the system namespace. */
    engine->localNamespace = engine->globalNamespace = engine->systemNamespace;

    /* Initialize application function definitions. */
    AspRunResult functionResult = AspInitializeAppFunctions(engine);
    if (functionResult != AspRunResult_OK)
        return functionResult;

    return AspRunResult_OK;
}

bool AspIsRunning(const AspEngine *engine)
{
    return engine->state == AspEngineState_Running;
}

uint32_t AspProgramCounter(const AspEngine *engine)
{
    return (uint32_t)(engine->pc - engine->code);
}

uint32_t AspLowFreeCount(const AspEngine *engine)
{
    return engine->lowFreeCount;
}
