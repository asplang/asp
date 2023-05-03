/*
 * Asp engine API definitions.
 */

#ifndef ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

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

/* Result returned from AspAddCode and AspSeal. */
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

#ifdef __cplusplus
}
#endif

/* Internal types. */
#include <asp-priv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization. */
size_t AspDataEntrySize(void);
void AspEngineVersion(uint8_t version[4]);
AspRunResult AspInitialize
    (AspEngine *,
     void *code, size_t codeSize,
     void *data, size_t dataSize,
     const AspAppSpec *, void *context);
void AspCodeVersion(const AspEngine *, uint8_t version[4]);
size_t AspMaxCodeSize(const AspEngine *);
size_t AspMaxDataSize(const AspEngine *);
AspAddCodeResult AspAddCode
    (AspEngine *, const void *code, size_t codeSize);
AspAddCodeResult AspSeal(AspEngine *);
AspRunResult AspReset(AspEngine *);
AspRunResult AspSetArguments(AspEngine *, const char * const *);
AspRunResult AspSetArgumentsString(AspEngine *, const char *);

/* Execution control. */
AspRunResult AspRestart(AspEngine *);
AspRunResult AspStep(AspEngine *);
bool AspIsRunning(const AspEngine *);
size_t AspProgramCounter(const AspEngine *);
size_t AspLowFreeCount(const AspEngine *);
#ifdef ASP_DEBUG
void AspDump(const AspEngine *, FILE *);
#endif

/* API for use by application functions. */
bool AspIsNone(const AspDataEntry *);
bool AspIsEllipsis(const AspDataEntry *);
bool AspIsBoolean(const AspDataEntry *);
bool AspIsInteger(const AspDataEntry *);
bool AspIsFloat(const AspDataEntry *);
bool AspIsIntegral(const AspDataEntry *);
bool AspIsNumber(const AspDataEntry *);
bool AspIsNumeric(const AspDataEntry *);
bool AspIsRange(const AspDataEntry *);
bool AspIsString(const AspDataEntry *);
bool AspIsTuple(const AspDataEntry *);
bool AspIsList(const AspDataEntry *);
bool AspIsSequence(const AspDataEntry *);
bool AspIsSet(const AspDataEntry *);
bool AspIsDictionary(const AspDataEntry *);
bool AspIsType(const AspDataEntry *);
bool AspIsTrue(AspEngine *, const AspDataEntry *);
bool AspIntegerValue(const AspDataEntry *, int32_t *);
bool AspFloatValue(const AspDataEntry *, double *);
bool AspRangeValues
    (AspEngine *, const AspDataEntry *,
     int32_t *start, int32_t *end, int32_t *step);
bool AspStringValue
    (AspEngine *, const AspDataEntry *,
     size_t *size, char *buffer, size_t index, size_t bufferSize);
AspDataEntry *AspToString(AspEngine *, AspDataEntry *);
unsigned AspCount(const AspDataEntry *);
AspDataEntry *AspElement(AspEngine *, AspDataEntry *sequence, int index);
char AspStringElement(AspEngine *, const AspDataEntry *str, int index);
AspDataEntry *AspFind
    (AspEngine *, AspDataEntry *tree, const AspDataEntry *key);
AspDataEntry *AspNext(AspEngine *, AspDataEntry *iterator);
AspDataEntry *AspNewNone(AspEngine *);
AspDataEntry *AspNewEllipsis(AspEngine *);
AspDataEntry *AspNewBoolean(AspEngine *, bool);
AspDataEntry *AspNewInteger(AspEngine *, int32_t);
AspDataEntry *AspNewFloat(AspEngine *, double);
AspDataEntry *AspNewString(AspEngine *, const char *buffer, size_t bufferSize);
AspDataEntry *AspNewTuple(AspEngine *);
AspDataEntry *AspNewList(AspEngine *);
AspDataEntry *AspNewSet(AspEngine *);
AspDataEntry *AspNewDictionary(AspEngine *);
AspDataEntry *AspNewIterator(AspEngine *, AspDataEntry *iterable);
AspDataEntry *AspNewType(AspEngine *, const AspDataEntry *);
bool AspTupleAppend
    (AspEngine *, AspDataEntry *tuple, AspDataEntry *value, bool take);
bool AspListAppend
    (AspEngine *, AspDataEntry *list, AspDataEntry *value, bool take);
bool AspListInsert
    (AspEngine *, AspDataEntry *list,
     int index, AspDataEntry *value, bool take);
bool AspListErase(AspEngine *, AspDataEntry *list, int index);
bool AspStringAppend
    (AspEngine *, AspDataEntry *str,
     const char *buffer, size_t bufferSize);
bool AspSetInsert
    (AspEngine *, AspDataEntry *set, AspDataEntry *key, bool take);
bool AspSetErase(AspEngine *, AspDataEntry *set, AspDataEntry *key);
bool AspDictionaryInsert
    (AspEngine *, AspDataEntry *dictionary,
     AspDataEntry *key, AspDataEntry *value, bool take);
bool AspDictionaryErase
    (AspEngine *, AspDataEntry *dictionary, AspDataEntry *key);
void AspRef(AspEngine *, AspDataEntry *);
void AspUnref(AspEngine *, AspDataEntry *);
AspDataEntry *AspArguments(AspEngine *);
void *AspContext(const AspEngine *);
bool AspAgain(const AspEngine *);
AspRunResult AspAssert(AspEngine *, bool);

#ifdef __cplusplus
}
#endif

#endif
