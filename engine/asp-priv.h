/*
 * Asp engine private definitions.
 * Do not use these definitions in application function implementations.
 *
 * Copyright (c) 2024 Canadensys Aerospace Corporation.
 * See LICENSE.txt at https://bitbucket.org/asplang/asp for details.
 */

#ifndef ASP_PRIVATE_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_PRIVATE_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AspEngine AspEngine;
typedef union AspDataEntry AspDataEntry;
typedef struct AspCodePageEntry AspCodePageEntry;
typedef struct AspAppSpec AspAppSpec;

#ifdef __cplusplus
}
#endif

#include "asp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AspRunResult (AspDispatchFunction)
    (AspEngine *, int32_t symbol, AspDataEntry *ns,
     AspDataEntry **returnValue);

struct AspCodePageEntry
{
    uint32_t offset;
    int8_t age;
};

struct AspAppSpec
{
    const char *spec;
    unsigned specSize;
    uint32_t checkValue;
    AspDispatchFunction *dispatch;
};

typedef enum AspEngineState
{
    AspEngineState_Reset,
    AspEngineState_LoadingHeader,
    AspEngineState_LoadingCode,
    AspEngineState_LoadError,
    AspEngineState_Ready,
    AspEngineState_Running,
    AspEngineState_RunError,
    AspEngineState_Ended,
} AspEngineState;

#ifndef ASP_CACHED_CODE_PAGE_COUNT
#define ASP_CACHED_CODE_PAGE_COUNT 8
#endif
#if ASP_CACHED_CODE_PAGE_COUNT < 0 || ASP_CACHED_CODE_PAGE_COUNT >= 128
#error ASP_CACHED_CODE_PAGE_COUNT is out of range
#endif

struct AspEngine
{
    /* Application context. */
    void *context;

    /* Floating-point format conversion routine. */
    AspFloatConverter floatConverter;

    /* Engine state and status. */
    AspEngineState state;
    uint8_t headerIndex;
    AspAddCodeResult loadResult;
    bool again;
    AspRunResult runResult;

    /* Version information. */
    uint8_t version[4]; /* major, minor, patch, tweak */

    /* Code space. */
    uint8_t *codeArea, *code;
    size_t maxCodeSize, codeEndIndex;
    uint32_t pc;

    /* Code paging data. */
    uint8_t cachedCodePageCount, cachedCodePageIndex;
    bool codeEndKnown;
    size_t codePageSize;
    AspCodeReader codeReader;
    void *pagedCodeId;
    AspCodePageEntry cachedCodePages[ASP_CACHED_CODE_PAGE_COUNT];
    size_t codePageReadCount;

    /* Data space. */
    AspDataEntry *data;
    size_t dataEndIndex;
    size_t freeCount, lowFreeCount;
    uint32_t freeListIndex;

    /* Loop iteration limit for detecting potential cycles in data
       structures. */
    uint32_t cycleDetectionLimit;

    /* Singletons for commonly used values. */
    AspDataEntry *noneSingleton, *falseSingleton, *trueSingleton;

    /* Stack. */
    AspDataEntry *stackTop;
    unsigned stackCount;

    /* Modules namespace. */
    AspDataEntry *modules;

    /* System module. */
    AspDataEntry *systemModule;

    /* Current module. */
    AspDataEntry *module;

    /* Current namespaces. */
    AspDataEntry *systemNamespace, *globalNamespace, *localNamespace;

    /* Application specification (functions). */
    const AspAppSpec *appSpec;

    /* Application function call state. */
    bool inApp;
    int32_t appFunctionSymbol;
    AspDataEntry *appFunctionNamespace, *appFunctionReturnValue;

    #ifdef ASP_DEBUG
    FILE *traceFile;
    #endif
};

/* Definitions used by auto-generated application function support code. */
typedef struct
{
    AspRunResult result;
    AspDataEntry *value;
} AspParameterResult;

/* Functions used by auto-generated application function support code. */
ASP_API AspDataEntry *AspParameterValue
    (AspEngine *, AspDataEntry *ns, int32_t symbol);
ASP_API AspParameterResult AspGroupParameterValue
    (AspEngine *, AspDataEntry *ns, int32_t symbol, bool dictionary);

#ifdef __cplusplus
}
#endif

#endif
