/*
 * Asp engine API definitions.
 */

#ifndef ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#ifdef ASP_DEBUG
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Result returned from AspAddCode. */
typedef enum
{
    AspAddCodeResult_OK = 0x00,
    AspAddCodeResult_InvalidFormat = 0x01,
    AspAddCodeResult_InvalidCheckValue = 0x02,
    AspAddCodeResult_OutOfCodeMemory = 0x03,
    AspAddCodeResult_InvalidState = 0x04,
} AspAddCodeResult;

/* Result returned from AspInitialize, AspReset, and AspStep. */
typedef enum
{
    AspRunResult_OK = 0x00,
    AspRunResult_Complete = 0x01,
    AspRunResult_InitializationError = 0x02,
    AspRunResult_InvalidState = 0x03,
    AspRunResult_InvalidInstruction = 0x04,
    AspRunResult_InvalidEnd= 0x05,
    AspRunResult_BeyondEndOfCode = 0x06,
    AspRunResult_StackUnderflow = 0x07,
    AspRunResult_InvalidContext = 0x0A,
    AspRunResult_Redundant = 0x0B,
    AspRunResult_UnexpectedType = 0x0C,
    AspRunResult_NameNotFound = 0x10,
    AspRunResult_KeyNotFound = 0x11,
    AspRunResult_IndexOutOfRange = 0x12,
    AspRunResult_IteratorAtEnd = 0x13,
    AspRunResult_MalformedFunctionCall = 0x14,
    AspRunResult_UndefinedAppFunction = 0x15,
    AspRunResult_DivideByZero = 0x18,
    AspRunResult_OutOfDataMemory = 0x20,
    AspRunResult_Again = 0xFA,
    AspRunResult_InternalError = 0xFE,
    AspRunResult_NotImplemented = 0xFF,
    AspRunResult_Application = 0x100,
} AspRunResult;

#ifdef __cplusplus
}
#endif

/* Internal types. */
#include "asp-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization. */
size_t AspDataEntrySize(void);
AspRunResult AspInitialize
    (AspEngine *,
     void *code, size_t codeSize,
     void *data, size_t dataSize,
     AspAppSpec *, void *context);
AspAddCodeResult AspAddCode
    (AspEngine *, const char *code, size_t codeSize);
AspAddCodeResult AspSeal(AspEngine *);
AspRunResult AspReset(AspEngine *);

/* Execution control. */
AspRunResult AspRestart(AspEngine *);
AspRunResult AspStep(AspEngine *);
bool AspIsRunning(const AspEngine *);
uint32_t AspProgramCounter(const AspEngine *);
uint32_t AspLowFreeCount(const AspEngine *);
#ifdef ASP_DEBUG
void AspDump(const AspEngine *, FILE *);
#endif

/* API for use by application functions. */
bool AspIsNone(const AspDataEntry *);
bool AspIsEllipsis(const AspDataEntry *);
bool AspIsBoolean(const AspDataEntry *);
bool AspIsInteger(const AspDataEntry *);
bool AspIsFloat(const AspDataEntry *);
bool AspIsNumeric(const AspDataEntry *);
bool AspIsNumber(const AspDataEntry *);
bool AspIsRange(const AspDataEntry *);
bool AspIsString(const AspDataEntry *);
bool AspIsTuple(const AspDataEntry *);
bool AspIsList(const AspDataEntry *);
bool AspIsSet(const AspDataEntry *);
bool AspIsDictionary(const AspDataEntry *);
bool AspIsTrue(AspEngine *, const AspDataEntry *);
bool AspIntegerValue(const AspDataEntry *, int32_t *);
bool AspFloatValue(const AspDataEntry *, double *);
bool AspRangeValue
    (AspEngine *, const AspDataEntry *,
     int32_t *start, int32_t *end, int32_t *step);
bool AspStringValue
    (AspEngine *, const AspDataEntry *,
     size_t *size, char *buffer, size_t index, size_t bufferSize);
unsigned AspCount(const AspDataEntry *);
AspDataEntry *AspListElement(AspEngine *, AspDataEntry *list, unsigned index);
char AspStringElement(AspEngine *, AspDataEntry *str, unsigned index);
AspDataEntry *AspFind(AspEngine *, AspDataEntry *tree, AspDataEntry *key);
AspDataEntry *AspNewNone(AspEngine *);
AspDataEntry *AspNewEllipsis(AspEngine *);
AspDataEntry *AspNewBoolean(AspEngine *, bool);
AspDataEntry *AspNewInteger(AspEngine *, int32_t);
AspDataEntry *AspNewFloat(AspEngine *, double);
AspDataEntry *AspNewString(AspEngine *, const char *);
AspDataEntry *AspNewTuple(AspEngine *);
AspDataEntry *AspNewList(AspEngine *);
AspDataEntry *AspNewSet(AspEngine *);
AspDataEntry *AspNewDictionary(AspEngine *);
bool AspListAppend(AspEngine *, AspDataEntry *list, AspDataEntry *value);
bool AspStringAppend(AspEngine *, AspDataEntry *str, char);
bool AspSetInsert(AspEngine *, AspDataEntry *set, AspDataEntry *key);
bool AspDictionaryInsert
    (AspEngine *, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value);
void AspRef(AspEngine *, AspDataEntry *);
void AspUnref(AspEngine *, AspDataEntry *);
void *AspContext(const AspEngine *);
bool AspAgain(const AspEngine *);
AspRunResult AspAssert(AspEngine *, bool);

#ifdef __cplusplus
}
#endif

#endif
