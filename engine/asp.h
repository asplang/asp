/*
 * Asp engine API definitions.
 *
 * Copyright (c) 2023 Canadensys Aerospace Corporation.
 * See LICENSE.txt at https://bitbucket.org/asplang/asp for details.
 */

#ifndef ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <asp-api.h>
#include <asp-ver.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#ifdef ASP_DEBUG
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Result returned from AspAddCode, AspSeal, and AspSealCode. */
typedef enum
{
    AspAddCodeResult_OK = 0x00,
    AspAddCodeResult_InvalidFormat = 0x01,
    AspAddCodeResult_InvalidVersion = 0x02,
    AspAddCodeResult_InvalidCheckValue = 0x03,
    AspAddCodeResult_OutOfCodeMemory = 0x04,
    AspAddCodeResult_InvalidState = 0x08,
} AspAddCodeResult;

/* Result returned from AspInitialize, AspReset, and AspStep, among others. */
typedef enum
{
    AspRunResult_OK = 0x00,
    AspRunResult_Complete = 0x01,
    AspRunResult_InitializationError = 0x02,
    AspRunResult_InvalidState = 0x03,
    AspRunResult_InvalidInstruction = 0x04,
    AspRunResult_InvalidEnd = 0x05,
    AspRunResult_BeyondEndOfCode = 0x06,
    AspRunResult_StackUnderflow = 0x07,
    AspRunResult_InvalidContext = 0x0A,
    AspRunResult_Redundant = 0x0B,
    AspRunResult_UnexpectedType = 0x0C,
    AspRunResult_SequenceMismatch = 0x0D,
    AspRunResult_StringFormattingError = 0x0E,
    AspRunResult_InvalidFormatString = 0x0F,
    AspRunResult_NameNotFound = 0x10,
    AspRunResult_KeyNotFound = 0x11,
    AspRunResult_ValueOutOfRange = 0x12,
    AspRunResult_IteratorAtEnd = 0x13,
    AspRunResult_MalformedFunctionCall = 0x14,
    AspRunResult_UndefinedAppFunction = 0x15,
    AspRunResult_DivideByZero = 0x18,
    AspRunResult_OutOfDataMemory = 0x20,
    AspRunResult_Again = 0xFA,
    AspRunResult_Abort = 0xFB,
    AspRunResult_InternalError = 0xFE,
    AspRunResult_NotImplemented = 0xFF,
    AspRunResult_Application = 0x100,
} AspRunResult;

/* Floating-point translator type. */
typedef double (*AspFloatConverter)(uint8_t ieee754_binary64[8]);

#ifdef __cplusplus
}
#endif

/* Internal types. */
#include <asp-priv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization. */
ASP_API size_t AspDataEntrySize(void);
ASP_API void AspEngineVersion(uint8_t version[4]);
ASP_API AspRunResult AspInitialize
    (AspEngine *, void *code, size_t codeSize, void *data, size_t dataSize,
     const AspAppSpec *, void *context);
ASP_API AspRunResult AspInitializeEx
    (AspEngine *, void *code, size_t codeSize, void *data, size_t dataSize,
     const AspAppSpec *, void *context, AspFloatConverter);
ASP_API void AspCodeVersion(const AspEngine *, uint8_t version[4]);
ASP_API size_t AspMaxCodeSize(const AspEngine *);
ASP_API size_t AspMaxDataSize(const AspEngine *);
ASP_API AspAddCodeResult AspAddCode
    (AspEngine *, const void *code, size_t codeSize);
ASP_API AspAddCodeResult AspSeal(AspEngine *);
ASP_API AspAddCodeResult AspSealCode
    (AspEngine *, const void *code, size_t codeSize);
ASP_API AspRunResult AspReset(AspEngine *);
ASP_API AspRunResult AspSetArguments(AspEngine *, const char * const *);
ASP_API AspRunResult AspSetArgumentsString(AspEngine *, const char *);

/* Execution control. */
ASP_API AspRunResult AspRestart(AspEngine *);
ASP_API AspRunResult AspStep(AspEngine *);
ASP_API bool AspIsReady(const AspEngine *);
ASP_API bool AspIsRunning(const AspEngine *);
ASP_API bool AspIsRunnable(const AspEngine *);
ASP_API size_t AspProgramCounter(const AspEngine *);
ASP_API size_t AspLowFreeCount(const AspEngine *);
#ifdef ASP_DEBUG
ASP_API void AspDump(const AspEngine *, FILE *);
#endif

/* API for use by application functions. */
ASP_API bool AspIsNone(const AspDataEntry *);
ASP_API bool AspIsEllipsis(const AspDataEntry *);
ASP_API bool AspIsBoolean(const AspDataEntry *);
ASP_API bool AspIsInteger(const AspDataEntry *);
ASP_API bool AspIsFloat(const AspDataEntry *);
ASP_API bool AspIsIntegral(const AspDataEntry *);
ASP_API bool AspIsNumber(const AspDataEntry *);
ASP_API bool AspIsNumeric(const AspDataEntry *);
ASP_API bool AspIsRange(const AspDataEntry *);
ASP_API bool AspIsString(const AspDataEntry *);
ASP_API bool AspIsTuple(const AspDataEntry *);
ASP_API bool AspIsList(const AspDataEntry *);
ASP_API bool AspIsSequence(const AspDataEntry *);
ASP_API bool AspIsSet(const AspDataEntry *);
ASP_API bool AspIsDictionary(const AspDataEntry *);
ASP_API bool AspIsAppIntegerObject(const AspDataEntry *);
ASP_API bool AspIsAppPointerObject(const AspDataEntry *);
ASP_API bool AspIsAppObject(const AspDataEntry *);
ASP_API bool AspIsType(const AspDataEntry *);
ASP_API bool AspIsTrue(AspEngine *, const AspDataEntry *);
ASP_API bool AspIntegerValue(const AspDataEntry *, int32_t *);
ASP_API bool AspFloatValue(const AspDataEntry *, double *);
ASP_API bool AspRangeValues
    (AspEngine *, const AspDataEntry *,
     int32_t *start, int32_t *end, int32_t *step);
ASP_API bool AspStringValue
    (AspEngine *, const AspDataEntry *,
     size_t *size, char *buffer, size_t index, size_t bufferSize);
ASP_API AspDataEntry *AspToString(AspEngine *, AspDataEntry *);
ASP_API AspDataEntry *AspToRepr(AspEngine *, const AspDataEntry *);
ASP_API unsigned AspCount(const AspDataEntry *);
ASP_API AspDataEntry *AspElement
    (AspEngine *, AspDataEntry *sequence, int index);
ASP_API char AspStringElement
    (AspEngine *, const AspDataEntry *str, int index);
ASP_API AspDataEntry *AspFind
    (AspEngine *, AspDataEntry *tree, const AspDataEntry *key);
ASP_API AspDataEntry *AspNext(AspEngine *, AspDataEntry *iterator);
ASP_API bool AspAppObjectTypeValue(const AspDataEntry *, int32_t *);
ASP_API bool AspAppIntegerObjectValues
    (const AspDataEntry *, int32_t *appType, int32_t *value);
ASP_API bool AspAppPointerObjectValues
    (const AspDataEntry *, int32_t *appType, void **valuePointer);
ASP_API AspDataEntry *AspNewNone(AspEngine *);
ASP_API AspDataEntry *AspNewEllipsis(AspEngine *);
ASP_API AspDataEntry *AspNewBoolean(AspEngine *, bool);
ASP_API AspDataEntry *AspNewInteger(AspEngine *, int32_t);
ASP_API AspDataEntry *AspNewFloat(AspEngine *, double);
ASP_API AspDataEntry *AspNewRange
    (AspEngine *, int32_t start, int32_t end, int32_t step);
ASP_API AspDataEntry *AspNewUnboundedRange
    (AspEngine *, int32_t start, int32_t step);
ASP_API AspDataEntry *AspNewString
    (AspEngine *, const char *buffer, size_t bufferSize);
ASP_API AspDataEntry *AspNewTuple(AspEngine *);
ASP_API AspDataEntry *AspNewList(AspEngine *);
ASP_API AspDataEntry *AspNewSet(AspEngine *);
ASP_API AspDataEntry *AspNewDictionary(AspEngine *);
ASP_API AspDataEntry *AspNewIterator(AspEngine *, AspDataEntry *iterable);
ASP_API AspDataEntry *AspNewAppIntegerObject
    (AspEngine *, int32_t appType, int32_t value);
ASP_API AspDataEntry *AspNewAppPointerObject
    (AspEngine *, int32_t appType, void *valuePointer);
ASP_API AspDataEntry *AspNewType(AspEngine *, const AspDataEntry *);
ASP_API bool AspTupleAppend
    (AspEngine *, AspDataEntry *tuple, AspDataEntry *value, bool take);
ASP_API bool AspListAppend
    (AspEngine *, AspDataEntry *list, AspDataEntry *value, bool take);
ASP_API bool AspListInsert
    (AspEngine *, AspDataEntry *list,
     int index, AspDataEntry *value, bool take);
ASP_API bool AspListErase(AspEngine *, AspDataEntry *list, int index);
ASP_API bool AspStringAppend
    (AspEngine *, AspDataEntry *str,
     const char *buffer, size_t bufferSize);
ASP_API bool AspSetInsert
    (AspEngine *, AspDataEntry *set, AspDataEntry *key, bool take);
ASP_API bool AspSetErase(AspEngine *, AspDataEntry *set, AspDataEntry *key);
ASP_API bool AspDictionaryInsert
    (AspEngine *, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value, bool take);
ASP_API bool AspDictionaryErase
    (AspEngine *, AspDataEntry *dictionary, AspDataEntry *key);
ASP_API void AspRef(AspEngine *, AspDataEntry *);
ASP_API void AspUnref(AspEngine *, AspDataEntry *);
ASP_API AspDataEntry *AspArguments(AspEngine *);
ASP_API void *AspContext(const AspEngine *);
ASP_API bool AspAgain(const AspEngine *);
ASP_API AspRunResult AspAssert(AspEngine *, bool);

#ifdef __cplusplus
}
#endif

#endif
