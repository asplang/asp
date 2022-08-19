/*
 * Asp engine private definitions.
 * Do not use these definitions in application function implementations.
 */

#ifndef ASP_PRIVATE_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_PRIVATE_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AspEngine AspEngine;
typedef struct AspDataEntry AspDataEntry;
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

typedef struct AspAppSpec
{
    char *spec;
    unsigned specSize;
    uint32_t checkValue;
    AspDispatchFunction *dispatch;
} AspAppSpec;

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

typedef struct AspEngine
{
    /* Application context. */
    void *context;

    /* Engine state and status. */
    AspEngineState state;
    uint8_t headerIndex;
    AspAddCodeResult loadResult;
    AspRunResult runResult;

    /* Code space. */
    uint8_t *code, *pc;
    uint32_t maxCodeSize, codeEndIndex;

    /* Data space. */
    AspDataEntry *data;
    uint32_t dataEndIndex;
    uint32_t freeCount, lowFreeCount;
    uint32_t freeListIndex;

    /* Singleton for None value. */
    AspDataEntry *noneSingleton;

    /* Stack. */
    AspDataEntry *stackTop;
    unsigned stackCount;

    /* Modules. */
    AspDataEntry *modules;

    /* Current module. */
    AspDataEntry *module;

    /* Current name spaces. */
    AspDataEntry *systemNamespace, *globalNamespace, *localNamespace;

    /* Application specification (functions). */
    AspAppSpec *appSpec;

    /* Application function call state. */
    bool inApp;

} AspEngine;

/* Definitions used by auto-generated application function support code. */
AspDataEntry *AspParameterValue
    (AspEngine *, AspDataEntry *ns, int32_t symbol);

#ifdef __cplusplus
}
#endif

#endif
