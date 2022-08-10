/*
 * Asp engine bit field implementation.
 */

#include "bits.h"

void AspBitSetField
    (uint32_t *word, unsigned shift, unsigned width, uint32_t value)
{
    uint32_t one = 1, mask = ((one << width) - one) << shift;
    *word = *word & ~mask | (value << shift) & mask;
}

uint32_t AspBitGetField
    (uint32_t word, unsigned shift, unsigned width)
{
    uint32_t one = 1, mask = ((one << width) - one);
    return (word >> shift) & mask;
}

void AspBitSetSignedField
    (uint32_t *word, unsigned shift, unsigned width, int32_t value)
{
    AspBitSetField(word, shift, width, *(uint32_t *)&value);
}

int32_t AspBitGetSignedField
    (uint32_t word, unsigned shift, unsigned width)
{
    uint32_t value = AspBitGetField(word, shift, width);
    if (AspBitGet(value, width - 1) != 0)
        AspBitSetField(&value, width, 32U - width, 0xFFFF);
    return *(int32_t *)&value;
}

void AspBitSet
    (uint32_t *word, unsigned shift, unsigned value)
{
    AspBitSetField(word, shift, 1, value);
}

uint32_t AspBitGet
    (uint32_t word, unsigned shift)
{
    return AspBitGetField(word, shift, 1);
}
