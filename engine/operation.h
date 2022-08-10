/*
 * Asp engine operations definitions.
 */

#ifndef ASP_OPERATION_H
#define ASP_OPERATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "asp-priv.h"

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
AspOperationResult AspPerformTernaryOperation
    (AspEngine *engine, uint8_t opCode,
     AspDataEntry *condition,
     AspDataEntry *falseValue, AspDataEntry *trueValue);

#ifdef __cplusplus
}
#endif

#endif
