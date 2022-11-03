/*
 * Asp script function library implementation: type functions.
 */

#include "asp.h"
#include "data.h"
#include <stdio.h>

/* type(object)
 * Return type of argument.
 */
AspRunResult AspLib_type
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

/* len(object)
 * Return length of object. Return 1 if object is not a container.
 */
AspRunResult AspLib_len
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
AspRunResult AspLib_bool
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

/* int(x)
 * Convert a number or string to an integer.
 * Floats are truncated towards zero. Out of range floats are converted to
 * either INT_MIN or INT_MAX according to the sign.
 * Strings are treated in the normal C way.
 *
 * TODO: Add base parameter for string version, supporting base 0 for usual
 * string interpretation.
 */
AspRunResult AspLib_int
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (AspIsInteger(x))
    {
        AspRef(engine, x);
        *returnValue = x;
        return AspRunResult_OK;
    }

    int32_t intValue;
    if (AspIsNumeric(x))
    {
        AspIntegerValue(x, &intValue);
    }
    else if (AspIsString(x))
    {
        size_t size;
        char buffer[12];
        AspStringValue(engine, x, &size, buffer, 0, sizeof buffer);
        if (size > 11)
            return AspRunResult_ValueOutOfRange;
        char c;
        int count = sscanf(buffer, "%d%c", &intValue, &c);
        if (count != 1)
            return AspRunResult_ValueOutOfRange;
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
AspRunResult AspLib_float
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
        size_t size;
        char buffer[25];
        AspStringValue(engine, x, &size, buffer, 0, sizeof buffer);
        if (size > 24)
            return AspRunResult_ValueOutOfRange;
        char c;
        int count = sscanf(buffer, "%lf%c", &floatValue, &c);
        if (count != 1)
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
AspRunResult AspLib_str
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    AspDataEntry *entry = AspToString(engine, object);
    if (entry == 0)
        return AspRunResult_OutOfDataMemory;
    *returnValue = entry;
    return AspRunResult_OK;
}
