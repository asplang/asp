/*
 * Asp engine sequence definitions.
 */

#ifndef ASP_SEQUENCE_H
#define ASP_SEQUENCE_H

#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *element, *value;
} AspSequenceResult;

AspSequenceResult AspSequenceAppend
    (AspEngine *, AspDataEntry *sequence, AspDataEntry *value);
AspSequenceResult AspSequenceInsertByIndex
    (AspEngine *, AspDataEntry *sequence,
     int index, AspDataEntry *value);
bool AspSequenceErase
    (AspEngine *, AspDataEntry *sequence, int index, bool eraseValue);
bool AspSequenceEraseElement
    (AspEngine *, AspDataEntry *sequence, AspDataEntry *element,
     bool eraseValue);
AspSequenceResult AspSequenceIndex
    (AspEngine *, AspDataEntry *sequence, int index);
AspSequenceResult AspSequenceNext
    (AspEngine *, AspDataEntry *sequence, AspDataEntry *element, bool right);
AspRunResult AspStringAppendBuffer
    (AspEngine *, AspDataEntry *str, const char *buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif
