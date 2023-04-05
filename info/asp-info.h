/*
 * Asp info library definitions.
 */

#ifndef ASP_INFO_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_INFO_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AspSourceInfo AspSourceInfo;

typedef struct AspSourceLocation
{
    const char *fileName;
    unsigned line, column;
} AspSourceLocation;

const char *AspAddCodeResultToString(int result);
const char *AspRunResultToString(int result);
AspSourceInfo *AspLoadSourceInfoFromFile(const char *fileName);
AspSourceInfo *AspLoadSourceInfo(const char *data, size_t size);
void AspUnloadSourceInfo(AspSourceInfo *);
AspSourceLocation AspGetSourceLocation(const AspSourceInfo *, size_t pc);
const char *AspGetSourceFileName(const AspSourceInfo *, unsigned index);

#ifdef __cplusplus
}
#endif

#endif
