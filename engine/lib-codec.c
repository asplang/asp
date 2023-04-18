/*
 * Asp script function library implementation: encode/decode functions.
 */

#include "asp.h"

static AspRunResult encode_f32
    (AspEngine *,
     AspDataEntry *x, bool reverse,
     AspDataEntry **returnValue);
static AspRunResult encode_f64
    (AspEngine *,
     AspDataEntry *x, bool reverse,
     AspDataEntry **returnValue);
static AspRunResult decode_i8
    (AspEngine *engine,
     AspDataEntry *s, bool extendSign,
     AspDataEntry **returnValue);
static AspRunResult decode_i16
    (AspEngine *engine,
     AspDataEntry *s, bool reverse, bool extendSign,
     AspDataEntry **returnValue);
static AspRunResult decode_i32
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue);
static AspRunResult decode_f32
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue);
static AspRunResult decode_f64
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue);
static bool be(void);
static void swap(uint8_t *, uint8_t *);

AspRunResult AspLib_encode_i8
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsInteger(x))
        return AspRunResult_UnexpectedType;

    uint32_t value;
    AspIntegerValue(x, (int32_t *)&value);
    uint8_t byte = value & 0xFF;
    *returnValue = AspNewString(engine, (const char *)&byte, 1);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_i16be
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsInteger(x))
        return AspRunResult_UnexpectedType;

    uint32_t value;
    AspIntegerValue(x, (int32_t *)&value);
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    *returnValue = AspNewString(engine, (const char *)bytes, 2);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_i16le
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsInteger(x))
        return AspRunResult_UnexpectedType;

    uint32_t value;
    AspIntegerValue(x, (int32_t *)&value);
    uint8_t bytes[2];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    *returnValue = AspNewString(engine, (const char *)bytes, 2);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_i32be
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsInteger(x))
        return AspRunResult_UnexpectedType;

    uint32_t value;
    AspIntegerValue(x, (int32_t *)&value);
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    *returnValue = AspNewString(engine, (const char *)bytes, 4);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_i32le
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsInteger(x))
        return AspRunResult_UnexpectedType;

    uint32_t value;
    AspIntegerValue(x, (int32_t *)&value);
    uint8_t bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    *returnValue = AspNewString(engine, (const char *)bytes, 4);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_f32be
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return encode_f32(engine, x, !be(), returnValue);
}

AspRunResult AspLib_encode_f32le
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return encode_f32(engine, x, be(), returnValue);
}

static AspRunResult encode_f32
    (AspEngine *engine,
     AspDataEntry *x, bool reverse,
     AspDataEntry **returnValue)
{
    if (!AspIsNumber(x))
        return AspRunResult_UnexpectedType;

    double value;
    AspFloatValue(x, &value);
    float f32 = (float)value;
    uint8_t *bytes = (uint8_t *)&f32;
    if (reverse)
    {
        swap(bytes + 0, bytes + 3);
        swap(bytes + 1, bytes + 2);
    }
    *returnValue = AspNewString(engine, (const char *)bytes, 4);

    return AspRunResult_OK;
}

AspRunResult AspLib_encode_f64be
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    static const uint16_t word = 1;
    bool be = *(const char *)&word == 0;
    return encode_f64(engine, x, !be, returnValue);
}

AspRunResult AspLib_encode_f64le
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    static const uint16_t word = 1;
    bool be = *(const char *)&word == 0;
    return encode_f64(engine, x, be, returnValue);
}

static AspRunResult encode_f64
    (AspEngine *engine,
     AspDataEntry *x, bool reverse,
     AspDataEntry **returnValue)
{
    if (!AspIsNumber(x))
        return AspRunResult_UnexpectedType;

    double value;
    AspFloatValue(x, &value);
    uint8_t *bytes = (uint8_t *)&value;
    if (reverse)
    {
        swap(bytes + 0, bytes + 7);
        swap(bytes + 1, bytes + 6);
        swap(bytes + 2, bytes + 5);
        swap(bytes + 3, bytes + 4);
    }
    *returnValue = AspNewString(engine, (const char *)bytes, 8);

    return AspRunResult_OK;
}

AspRunResult AspLib_decode_i8
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i8(engine, s, true, returnValue);
}

AspRunResult AspLib_decode_u8
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i8(engine, s, false, returnValue);
}

static AspRunResult decode_i8
    (AspEngine *engine,
     AspDataEntry *s, bool extendSign,
     AspDataEntry **returnValue)
{
    if (!AspIsString(s))
        return AspRunResult_UnexpectedType;
    if (AspCount(s) != 1)
        return AspRunResult_ValueOutOfRange;

    uint8_t byte;
    AspStringValue(engine, s, 0, (char *)&byte, 0, 1);
    int32_t value;
    if (extendSign)
        value = *(int8_t *)&byte;
    else
    {
        uint32_t unsignedValue = byte;
        value = *(int32_t *)&unsignedValue;
    }
    *returnValue = AspNewInteger(engine, value);

    return AspRunResult_OK;
}

AspRunResult AspLib_decode_i16be
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i16(engine, s, !be(), true, returnValue);
}

AspRunResult AspLib_decode_i16le
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i16(engine, s, be(), true, returnValue);
}

AspRunResult AspLib_decode_u16be
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i16(engine, s, !be(), false, returnValue);
}

AspRunResult AspLib_decode_u16le
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i16(engine, s, be(), false, returnValue);
}

static AspRunResult decode_i16
    (AspEngine *engine,
     AspDataEntry *s, bool reverse, bool extendSign,
     AspDataEntry **returnValue)
{
    if (!AspIsString(s))
        return AspRunResult_UnexpectedType;
    if (AspCount(s) != 2)
        return AspRunResult_ValueOutOfRange;

    uint8_t bytes[2];
    AspStringValue(engine, s, 0, (char *)bytes, 0, 2);
    if (reverse)
        swap(bytes + 0, bytes + 1);
    int32_t value;
    if (extendSign)
        value = *(int16_t *)bytes;
    else
    {
        uint32_t unsignedValue = *(uint16_t *)bytes;
        value = *(int32_t *)&unsignedValue;
    }
    *returnValue = AspNewInteger(engine, value);

    return AspRunResult_OK;
}

AspRunResult AspLib_decode_i32be
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i32(engine, s, !be(), returnValue);
}

AspRunResult AspLib_decode_i32le
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_i32(engine, s, be(), returnValue);
}

static AspRunResult decode_i32
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue)
{
    if (!AspIsString(s))
        return AspRunResult_UnexpectedType;
    if (AspCount(s) != 4)
        return AspRunResult_ValueOutOfRange;

    uint8_t bytes[4];
    AspStringValue(engine, s, 0, (char *)bytes, 0, 4);
    if (reverse)
    {
        swap(bytes + 0, bytes + 3);
        swap(bytes + 1, bytes + 2);
    }
    int32_t value = *(int32_t *)bytes;
    *returnValue = AspNewInteger(engine, value);

    return AspRunResult_OK;
}

AspRunResult AspLib_decode_f32be
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_f32(engine, s, !be(), returnValue);
}

AspRunResult AspLib_decode_f32le
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_f32(engine, s, be(), returnValue);
}

static AspRunResult decode_f32
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue)
{
    if (!AspIsString(s))
        return AspRunResult_UnexpectedType;
    if (AspCount(s) != 4)
        return AspRunResult_ValueOutOfRange;

    uint8_t bytes[4];
    AspStringValue(engine, s, 0, (char *)bytes, 0, 4);
    if (reverse)
    {
        swap(bytes + 0, bytes + 3);
        swap(bytes + 1, bytes + 2);
    }
    float value = *(float *)bytes;
    *returnValue = AspNewFloat(engine, value);

    return AspRunResult_OK;
}

AspRunResult AspLib_decode_f64be
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_f64(engine, s, !be(), returnValue);
}

AspRunResult AspLib_decode_f64le
    (AspEngine *engine,
     AspDataEntry *s,
     AspDataEntry **returnValue)
{
    return decode_f64(engine, s, be(), returnValue);
}

static AspRunResult decode_f64
    (AspEngine *engine,
     AspDataEntry *s, bool reverse,
     AspDataEntry **returnValue)
{
    if (!AspIsString(s))
        return AspRunResult_UnexpectedType;
    if (AspCount(s) != 8)
        return AspRunResult_ValueOutOfRange;

    uint8_t bytes[8];
    AspStringValue(engine, s, 0, (char *)bytes, 0, 8);
    if (reverse)
    {
        swap(bytes + 0, bytes + 7);
        swap(bytes + 1, bytes + 6);
        swap(bytes + 2, bytes + 5);
        swap(bytes + 3, bytes + 4);
    }
    double value = *(double *)bytes;
    *returnValue = AspNewFloat(engine, value);

    return AspRunResult_OK;
}

static bool be(void)
{
    static const uint16_t word = 1;
    return *(const char *)&word == 0;
}

static void swap(uint8_t *a, uint8_t *b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}
