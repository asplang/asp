/*
 * Asp engine operations definitions.
 */

#ifndef ASP_OPERATION_H
#define ASP_OPERATION_H

#include "asp-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *value;
} AspOperationResult;

AspOperationResult AspPerformUnaryOperation
    (AspEngine *engine, uint8_t opCode, AspDataEntry *operand);
AspOperationResult AspPerformBinaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *left, AspDataEntry *right);

#ifdef __cplusplus
}
#endif

#endif
