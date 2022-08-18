/*
 * Cyclic Redundancy Code (CRC) calculator implementation.
 *
 * Notes: This implementation supports polynomial widths less than 8 and
 * up to 32 or 64, depending on whether CRC_SUPPORT_64 is defined.
 * When the code is compiled with CRC_SUPPORT_64, calculations using
 * widths of 32 bits or less use 32-bit values where possible for the
 * sake of efficiency.
 */

#include "crc.h"

#ifdef CRC_SUPPORT_64
#define ASSIGN_EQ(lhs, rhs) \
    {if (spec->width > 32) {(lhs)._64 = (rhs);} else {(lhs)._32 = (rhs);}}
#define ASSIGN_PREFIX(lhs, prefix, rhs) {if (spec->width > 32) \
    {(lhs)._64 prefix ## = (rhs);} else {(lhs)._32 prefix ## = (rhs);}}
#define ACCESS(v) (spec->width > 32 ? (v)._64 : (v)._32)
#else
#define ASSIGN_EQ(lhs, rhs) ((lhs) = (rhs))
#define ASSIGN_PREFIX(lhs, prefix, rhs) ((lhs) prefix ## = (rhs))
#define ACCESS(v) (v)
#endif
#define ASSIGN_N(_1, _2, _3, arg, ...) arg
#define ASSIGN(...) ASSIGN_N(__VA_ARGS__, ASSIGN_PREFIX, ASSIGN_EQ)(__VA_ARGS__)

static crc_arg reflect(crc_arg value, unsigned bit_count);

void crc_init_spec
    (uint8_t width, crc_arg poly,
     crc_arg init, bool refin, bool refout, crc_arg xorout,
     crc_spec *spec)
{
    crc_value spoly;

    spec->width = width;
    spec->shift = 0;
    ASSIGN(spoly, poly);
    ASSIGN(spec->init, init);
    spec->refin = refin;
    spec->refout = refout;
    ASSIGN(spec->xorout, xorout);
    ASSIGN(spec->mask,
        width == 64 ? (uint64_t)(int64_t)(-1) :
        width > 32 ? ((uint64_t)1U << width) - 1U :
        width == 32 ? (uint32_t)(int32_t)(-1) :
        ((uint32_t)1U << width) - 1U);

    /* For widths less than 1 byte, shift things to the left so that the
       action happens at the byte's MSb. The final answer will be shifted
       back. */
    if (width < 8)
    {
        spec->width = 8;
        spec->shift = 8 - width;
        ASSIGN(spoly, <<, spec->shift);
        ASSIGN(spec->init, <<, spec->shift);
        ASSIGN(spec->mask, <<, spec->shift);
    }

    /* Initialize tables. */
    crc_arg top_bit_mask = (crc_arg)1U << (spec->width - 1);
    for (unsigned b = 0; b < 0x100; b++)
    {
        /* Reflect the byte for use here and during calculation. */
        uint8_t ref_b = reflect(b, 8);

        /* Determine CRC contribution for the byte. */
        crc_arg crc = 0;
        uint8_t byte_value = refin ? b : ref_b;
        for (unsigned bit_index = 0; bit_index < 8; bit_index++)
        {
            crc ^= (crc_arg)byte_value << (spec->width - 1);

            bool topSet = crc & top_bit_mask;
            crc <<= 1;
            if (topSet)
                crc ^= ACCESS(spoly);

            byte_value >>= 1;
        }

        /* Initialize the applicable table entries. */
        unsigned crc_index = refin ? ref_b : b;
        spec->byte_table[b] = crc_index;
        ASSIGN(spec->crc_table[crc_index], crc & ACCESS(spec->mask));
    }
}

crc_spec crc_make_spec
    (uint8_t width, crc_arg poly,
     crc_arg init, bool refin, bool refout, crc_arg xorout)
{
    crc_spec spec;
    crc_init_spec(width, poly, init, refin, refout, xorout, &spec);
    return spec;
}

crc_arg crc_calc(const crc_spec *spec, const void *buffer, unsigned size)
{
    crc_session session;
    crc_start(spec, &session);
    crc_add(spec, &session, buffer, size);
    return crc_finish(spec, &session);
}

void crc_start(const crc_spec *spec, crc_session *session)
{
    ASSIGN(session->crc, ACCESS(spec->init));
}

void crc_add
    (const crc_spec *spec, crc_session *session,
     const void *buffer, unsigned size)
{
    const uint8_t *bp = (const uint8_t *)buffer;

    /* Use the tables to process the buffer 1 byte at a time. */
    while (size--)
    {
        uint8_t b = spec->byte_table[*bp++];
        uint8_t index = b ^ (ACCESS(session->crc) >> (spec->width - 8));
        ASSIGN(session->crc,
            (ACCESS(session->crc) << 8) ^ ACCESS(spec->crc_table[index]));
    }

    ASSIGN(session->crc, &, ACCESS(spec->mask));
}

crc_arg crc_finish(const crc_spec *spec, crc_session *session)
{
    /* Determine the final result, shifting right for widths less than
       1 byte. */
    crc_arg result = ACCESS(session->crc) >> spec->shift;
    if (spec->refout)
        result = reflect(result, spec->width - spec->shift);
    result ^= ACCESS(spec->xorout);
    return result & (ACCESS(spec->mask) >> spec->shift);
}

static crc_arg reflect(crc_arg value, unsigned bit_count)
{
    crc_arg result = 0;
    for (uint8_t bit_index = 0; bit_index < bit_count; bit_index++, value >>= 1)
    {
        if (value & 1UL)
            result |= 1UL << ((bit_count - 1) - bit_index);
    }

    return result;
}
