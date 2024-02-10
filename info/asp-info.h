/*
 * Asp info library definitions.
 *
 * Copyright (c) 2024 Canadensys Aerospace Corporation.
 * See LICENSE.txt at https://bitbucket.org/asplang/asp for details.
 */

#ifndef ASP_INFO_080177a8_14ce_11ed_b65f_7328ac4c64a3_H
#define ASP_INFO_080177a8_14ce_11ed_b65f_7328ac4c64a3_H

#include <asp-api.h>
#include <stdint.h>
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

ASP_API const char *AspAddCodeResultToString(int result);
ASP_API const char *AspRunResultToString(int result);
ASP_API AspSourceInfo *AspLoadSourceInfoFromFile
    (const char *fileName);
ASP_API AspSourceInfo *AspLoadSourceInfo
    (const char *data, size_t size);
ASP_API void AspUnloadSourceInfo(AspSourceInfo *);
ASP_API AspSourceLocation AspGetSourceLocation
    (const AspSourceInfo *, uint32_t pc);
ASP_API const char *AspGetSourceFileName
    (const AspSourceInfo *, unsigned index);

#ifdef __cplusplus
}
#endif

#endif
