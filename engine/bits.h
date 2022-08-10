/*
 * Asp engine bit field definitions.
 */

#ifndef ASP_BITS_H
#define ASP_BITS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AspBitSetField
    (uint32_t *word, unsigned shift, unsigned width, uint32_t value);
uint32_t AspBitGetField
    (uint32_t word, unsigned shift, unsigned width);
void AspBitSetSignedField
    (uint32_t *word, unsigned shift, unsigned width, int32_t value);
int32_t AspBitGetSignedField
    (uint32_t word, unsigned shift, unsigned width);
void AspBitSet
    (uint32_t *word, unsigned shift, uint32_t value);
uint32_t AspBitGet
    (uint32_t word, unsigned shift);

#ifdef __cplusplus
}
#endif

#endif
