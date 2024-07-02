/*
 * Asp engine code definitions.
 */

#ifndef ASP_CODE_H
#define ASP_CODE_H

#include "asp.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

AspRunResult AspLoadCodeBytes(AspEngine *, uint8_t *bytes, size_t count);
AspRunResult AspValidateCodeAddress(AspEngine *, uint32_t address);
AspRunResult AspLoadCodePage(AspEngine *, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif
