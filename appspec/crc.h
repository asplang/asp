/*
 * Cyclic Redundancy Code (CRC) calculator definitions.
 *
 * Supports CRCs up to 32 bits wide by default.
 * Define CRC_SUPPORT_64 to support widths up to 64 bits.
 *
 * Some common (and not so common) CRCs:
 *   Name                   Initialization input parameters
 *   ----                   -------------------------------
 *   CRC-16/ARC             (16, 0x8005, 0, true, true, 0)
 *   XMODEM                 (16, 0x1021, 0, false, false, 0)
 *   Kermit                 (16, 0x1021, 0, true, true, 0)
 *   X.25                   (16, 0x1021, 0xFFFF, true, true, 0xFFFF);
 *   CRC-24/FlexRay-B       (24, 0x5D6DCB, 0xABCDEF, false, false, 0)
 *   CRC-31/Philips         (31, 0x04C11DB7, 0xFFFFFFFF, false, false, 0xFFFFFFFF)
 *   CRC-32/ISO-HDLC        (32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF)
 *   CRC-32/ISCSI           (32, 0x1EDC6F41, 0xFFFFFFFF, true, true, 0xFFFFFFFF)
 *
 * See also https://reveng.sourceforge.io. Full cataloge can be found at
 * https://reveng.sourceforge.io/crc-catalogue.
 */

#ifndef CRC_H
#define CRC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CRC_SUPPORT_64
typedef union {uint32_t _32; uint64_t _64;} crc_value;
typedef uint64_t crc_arg;
#else
typedef uint32_t crc_value;
typedef uint32_t crc_arg;
#endif

typedef struct crc_spec_t
{
    uint8_t width;
    uint8_t shift;
    bool refin, refout;
    crc_value mask;
    uint8_t byte_table[0x100];
    crc_value crc_table[0x100];
    crc_value init, xorout;
} crc_spec;

typedef struct crc_session_t
{
    crc_value crc;
} crc_session;

void crc_init_spec
    (uint8_t width, crc_arg poly,
     crc_arg init, bool refin, bool refout, crc_arg xorout,
     crc_spec *);
crc_spec crc_make_spec
    (uint8_t width, crc_arg poly,
     crc_arg init, bool refin, bool refout, crc_arg xorout);
crc_arg crc_calc(const crc_spec *, const void *buffer, size_t size);
void crc_start(const crc_spec *, crc_session *session);
void crc_add
    (const crc_spec *, crc_session *,
     const void *buffer, size_t size);
crc_arg crc_finish(const crc_spec *, crc_session *);

#ifdef __cplusplus
}
#endif

#endif
