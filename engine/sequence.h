/*
 * Asp engine sequence definitions.
 */

#ifndef ASP_SEQUENCE_H
#define ASP_SEQUENCE_H

#include "asp-priv.h"
#include "data.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    AspRunResult result;
    AspDataEntry *element, *value;
} AspSequenceResult;

AspSequenceResult AspSequenceAppend
    (AspEngine *, AspDataEntry *list, AspDataEntry *value);
AspSequenceResult AspSequenceInsertByIndex
    (AspEngine *, AspDataEntry *list,
     int index, AspDataEntry *value);
AspSequenceResult AspSequenceInsert
    (AspEngine *, AspDataEntry *list,
     AspDataEntry *element, AspDataEntry *value);
bool AspSequenceErase
    (AspEngine *, AspDataEntry *list, int index, bool eraseValue);
bool AspSequenceEraseElement
    (AspEngine *, AspDataEntry *list, AspDataEntry *element, bool eraseValue);
AspSequenceResult AspSequenceIndex
    (AspEngine *, AspDataEntry *list, int index);
AspSequenceResult AspSequenceNext
    (AspEngine *, AspDataEntry *list, AspDataEntry *element);

#ifdef __cplusplus
}
#endif

#endif
