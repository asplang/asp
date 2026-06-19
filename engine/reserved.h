/*
 * Asp reserved names definitions.
 */

#ifndef ASP_RESERVED_H
#define ASP_RESERVED_H

#include "asp.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AspReservedSymbol
{
    AspReservedSymbol_SystemModule = 0,
    AspReservedSymbol_SystemArguments = 1,
    AspReservedSymbol_MainModule = 2,
    #ifdef ASP_FEATURE_CLASS
    AspReservedSymbol_ClassInitialize = 3,
    AspReservedSymbol_GetMethod = 4,
    AspReservedSymbol_CallMethod = 7,
    #endif

    AspReservedSymbol_End = 64 /* must be last */
};

typedef struct
{
    int32_t symbol;
    const char *name;
    AspFeatureBits featureBits;
} AspReservedNameEntry;

const char *AspReservedName(int32_t symbol);
const AspReservedNameEntry *AspNextReservedNameEntry
    (const AspReservedNameEntry *);

#ifdef __cplusplus
}
#endif

#endif
