/*
 * Asp engine predeinfed symbols definitions.
 */

#ifndef ASP_SYMBOLS_H
#define ASP_SYMBOLS_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AspSystemModuleSymbol 0
#define AspSystemArgumentsSymbol 1
#define AspSystemMainModuleSymbol 2
#define AspClassInitializeSymbol 3
#define AspScriptSymbolBase 64

#define AspIsSymbolReserved(symbol) \
    ((symbol) == AspSystemModuleSymbol || \
     (symbol) == AspSystemArgumentsSymbol || \
     (symbol) == AspSystemMainModuleSymbol || \
     (symbol) == AspClassInitializeSymbol)

#define AspSystemModuleName "sys"
#define AspSystemArgumentsName "args"
#define AspSystemMainModuleName "__main__"
#define AspClassInitializeName "__init__"

#define AspIsNameReserved(name) \
    (strcmp((name), AspSystemModuleName) == 0 || \
     strcmp((name), AspSystemArgumentsName) == 0 || \
     strcmp((name), AspSystemMainModuleName) == 0 || \
     strcmp((name), AspClassInitializeName) == 0)

#ifdef __cplusplus
}
#endif

#endif
