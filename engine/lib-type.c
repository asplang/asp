/*
 * Asp script function library implementation: type functions.
 */

#include "asp.h"
#include "data.h"
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

static AspRunResult ExtractWord
    (AspEngine *, AspDataEntry *str, char *, size_t *);

/* type(object)
 * Return type of argument.
 */
ASP_LIB_API AspRunResult AspLib_type
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    AspDataEntry *typeEntry = AspAllocEntry(engine, DataType_Type);
    if (typeEntry == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetTypeValue(typeEntry, AspDataGetType(object));
    *returnValue = typeEntry;
    return AspRunResult_OK;
}

/* key(x)
 * Return x if it can be used as a set/dictionary key, otherwise None.
 */
ASP_LIB_API AspRunResult AspLib_key
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    bool isImmutable;
    AspRunResult result = AspCheckIsImmutableObject
        (engine, object, &isImmutable);
    if (result != AspRunResult_OK)
        return result;
    if (isImmutable)
    {
        AspRef(engine, object);
        *returnValue = object;
    }
    return AspRunResult_OK;
}

/* len(object)
 * Return length of object. Return 1 if object is not a container.
 */
ASP_LIB_API AspRunResult AspLib_len
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    unsigned count = AspCount(object);
    int32_t len = *(int32_t *)&count;
    AspDataEntry *lenEntry = AspNewInteger(engine, len);
    if (lenEntry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = lenEntry;
    return AspRunResult_OK;
}

/* bool(x)
 * Return True if the argument is true, False otherwise.
 */
ASP_LIB_API AspRunResult AspLib_bool
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (AspIsBoolean(x))
    {
        AspRef(engine, x);
        *returnValue = x;
        return AspRunResult_OK;
    }

    AspDataEntry *entry = AspNewBoolean(engine, AspIsTrue(engine, x));
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}

/* int(x, base, check)
 * Convert a number or string to an integer.
 * Floats are truncated towards zero. Out of range floats (including infinities
 * and NaNs) raise an error condition if check is true. Otherwise, out of range
 * floats (including infinities) are converted to either INT32_MIN or INT32_MAX
 * according to the sign, and NaNs are converted to zero.
 * Strings are treated in the normal C way, as per strtol.
 * If base is given, x must be a string.
 * For string conversions, the default base is 10.
 */
ASP_LIB_API AspRunResult AspLib_int
    (AspEngine *engine,
     AspDataEntry *x, AspDataEntry *base, AspDataEntry *check,
     AspDataEntry **returnValue)
{
    if (AspIsInteger(x))
    {
        AspRef(engine, x);
        *returnValue = x;
        return AspRunResult_OK;
    }

    if (!AspIsNone(base) && !AspIsString(x))
        return AspRunResult_UnexpectedType;

    int32_t intValue;
    if (AspIsNumeric(x))
    {
        bool valid = AspIntegerValue(x, &intValue);
        if (AspIsTrue(engine, check) && !valid)
            return AspRunResult_ValueOutOfRange;
    }
    else if (AspIsString(x))
    {
        int32_t baseValue = 10;
        if (!AspIsNone(base))
        {
            if (!AspIsIntegral(base))
                return AspRunResult_UnexpectedType;
            AspIntegerValue(base, &baseValue);
            if (baseValue != 0 && (baseValue < 2 || baseValue > 36))
                return AspRunResult_ValueOutOfRange;
        }

        /* Extract the word to convert. */
        char buffer[12];
        size_t size = sizeof buffer;
        AspRunResult extractResult = ExtractWord(engine, x, buffer, &size);
        if (extractResult != AspRunResult_OK)
            return extractResult;

        /* Convert the word to an integer value. */
        char *endp;
        errno = 0;
        long longValue = strtol(buffer, &endp, (int)baseValue);
        if (*endp != '\0' || errno != 0 ||
            longValue < INT32_MIN || longValue > INT32_MAX)
            return AspRunResult_ValueOutOfRange;
        intValue = (int32_t)longValue;
    }
    else
        return AspRunResult_UnexpectedType;

    AspDataEntry *entry = AspNewInteger(engine, intValue);
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}

/* float(x)
 * Convert a number or string to a float.
 */
ASP_LIB_API AspRunResult AspLib_float
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (AspIsFloat(x))
    {
        AspRef(engine, x);
        *returnValue = x;
        return AspRunResult_OK;
    }

    double floatValue;
    if (AspIsNumeric(x))
    {
        AspFloatValue(x, &floatValue);
    }
    else if (AspIsString(x))
    {
        /* Extract the word to convert. */
        char buffer[25];
        size_t size = sizeof buffer;
        AspRunResult extractResult = ExtractWord(engine, x, buffer, &size);
        if (extractResult != AspRunResult_OK)
            return extractResult;

        /* Convert the word to a floating-point value. */
        char *endp;
        errno = 0;
        floatValue = strtod(buffer, &endp);
        if (*endp != '\0' || errno != 0)
            return AspRunResult_ValueOutOfRange;
    }
    else
        return AspRunResult_UnexpectedType;

    AspDataEntry *entry = AspNewFloat(engine, floatValue);
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}

/* str(x)
 * Return a string representation of x.
 */
ASP_LIB_API AspRunResult AspLib_str
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    AspDataEntry *entry = AspToString(engine, x);
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}

/* repr(x)
 * Return the canonical string representation of x.
 */
ASP_LIB_API AspRunResult AspLib_repr
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    AspDataEntry *entry = AspToRepr(engine, x);
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}

static AspRunResult ExtractWord
    (AspEngine *engine, AspDataEntry *str,
     char *buffer, size_t *bufferSize)
{
    size_t maxBufferSize = *bufferSize;
    *bufferSize = 0;
    bool end = false;
    for (int index = 0; index < (int)AspCount(str); index++)
    {
        char c = AspStringElement(engine, str, index);
        if (isspace(c))
        {
            if (*bufferSize != 0)
                end = true;
        }
        else
        {
            if (end || *bufferSize >= maxBufferSize - 1)
                return AspRunResult_ValueOutOfRange;
            buffer[(*bufferSize)++] = c;
        }
    }
    buffer[*bufferSize] = '\0';

    return AspRunResult_OK;
}
