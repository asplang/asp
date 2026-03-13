/*
 * Asp reserved names definitions.
 */

#ifndef ASP_RESERVED_H
#define ASP_RESERVED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AspReservedSymbol
{
    AspReservedSymbol_SystemModule,
    AspReservedSymbol_SystemArguments,
    AspReservedSymbol_MainModule,
    AspReservedSymbol_ClassInitialize,

    AspReservedSymbol_End = 64 /* must be last */
};

typedef struct
{
    int32_t symbol;
    const char *name;
} AspReservedNameEntry;

const char *AspReservedName(int32_t symbol);
const AspReservedNameEntry *AspNextReservedNameEntry
    (const AspReservedNameEntry *);

#ifdef __cplusplus
}
#endif

#endif
