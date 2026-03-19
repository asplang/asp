/*
 * Asp script function library implementation: type functions.
 */

#include "asp.h"
#include "data.h"
#include "stack.h"
#include "range.h"
#include "sequence.h"
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
    /* For class instances, return the class. */
    if (AspIsClassInstance(object))
    {
        *returnValue = AspInstanceClass(engine, object);
        AspRef(engine, *returnValue);
        return AspRunResult_OK;
    }

    AspDataEntry *typeEntry = AspAllocEntry(engine, DataType_Type);
    if (typeEntry == 0)
        return AspRunResult_OutOfDataMemory;
    AspDataSetTypeValue(typeEntry, AspDataGetType(object));
    *returnValue = typeEntry;
    return AspRunResult_OK;
}

/* isinstance(object, type)
 * Return True if the object is of the given type(s).
 */
ASP_LIB_API AspRunResult AspLib_isinstance
    (AspEngine *engine,
     AspDataEntry *object, AspDataEntry *type,
     AspDataEntry **returnValue)
{
    bool is = false;
    if (!AspIsTuple(type))
        is = AspIsTypeOf(engine, object, type);
    else
    {
        /* For tuples, we must examine the contents. Avoid recursion by using
           the the engine's stack. */
        bool isImmutable = true;
        const AspDataEntry *startStackTop = engine->stackTop;
        uint32_t iterationCount = 0;
        for (; iterationCount < engine->cycleDetectionLimit; iterationCount++)
        {
            uint32_t iterationCount = 0;
            for (AspSequenceResult nextResult = AspSequenceNext
                    (engine, type, 0, true);
                 iterationCount < engine->cycleDetectionLimit &&
                 nextResult.element != 0;
                 iterationCount++,
                 nextResult = AspSequenceNext
                    (engine, type, nextResult.element, true))
            {
                const AspDataEntry *value = nextResult.value;

                if (AspDataGetType(value) == DataType_Tuple)
                {
                    if (AspPushNoUse(engine, nextResult.value) == 0)
                        return AspRunResult_OutOfDataMemory;
                }
                else if (AspIsTypeOf(engine, object, value))
                {
                    is = true;
                    break;
                }
            }
            if (iterationCount >= engine->cycleDetectionLimit)
                return AspRunResult_CycleDetected;
            if (is)
                break;

            /* Check if there's more to do. */
            if (engine->stackTop == startStackTop ||
                engine->runResult != AspRunResult_OK)
                break;

            /* Fetch the next item from the stack. */
            type = AspTopValue(engine);
            AspPopNoErase(engine);
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;

        /* Unwind the working stack if necessary. */
        if (engine->runResult == AspRunResult_OK)
        {
            uint32_t iterationCount = 0;
            for (;
                 iterationCount < engine->cycleDetectionLimit &&
                 engine->stackTop != startStackTop;
                 iterationCount++)
            {
                AspPopNoErase(engine);
            }
            if (iterationCount >= engine->cycleDetectionLimit)
                return AspRunResult_CycleDetected;
        }
    }

    return
        (*returnValue = AspNewBoolean(engine, is)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* super(type, object)
 * Return a super object for calling methods in a base class.
 */
ASP_LIB_API AspRunResult AspLib_super
    (AspEngine *engine,
     AspDataEntry *cls, AspDataEntry *instance,
     AspDataEntry **returnValue)
{
    if (!AspIsNone(cls) || !AspIsNone(instance))
    {
        if (!AspIsClass(cls) || !AspIsClassInstance(instance) ||
            !AspIsTypeOf(engine, instance, cls))
            return AspRunResult_UnexpectedType;
    }
    else
    {
        /* Search for the enclosing frame. */
        uint32_t prevStackEntryIndex = AspDataGetStackEntryPreviousIndex
            (engine->stackTop);
        if (prevStackEntryIndex == 0)
            return AspRunResult_InvalidContext;
        AspDataEntry *frame = 0;
        uint32_t iterationCount = 0;
        AspDataEntry *stackTop = engine->stackTop;
        for (AspDataEntry *stackEntry = AspEntry(engine, prevStackEntryIndex);
             iterationCount < engine->cycleDetectionLimit && stackEntry != 0;
             iterationCount++, stackEntry =
             ((prevStackEntryIndex =
               AspDataGetStackEntryPreviousIndex(stackEntry)) != 0 ?
              AspEntry(engine, prevStackEntryIndex) : 0))
        {
            AspDataEntry *entry = AspValueEntry
                (engine, AspDataGetStackEntryValueIndex(stackEntry));
            if (AspDataGetType(entry) == DataType_Frame)
            {
                frame = entry;
                break;
            }
        }
        if (iterationCount >= engine->cycleDetectionLimit)
            return AspRunResult_CycleDetected;
        if (frame == 0)
            return AspRunResult_InvalidContext;
        uint32_t contextIndex = AspDataGetFrameContextIndex(frame);
        if (contextIndex == 0)
            return AspRunResult_InvalidContext;

        AspDataEntry *context = AspEntry(engine, contextIndex);
        uint32_t classIndex = AspDataGetContextClassIndex(context);
        if (classIndex == 0)
            return AspRunResult_InternalError;
        else
            cls = AspValueEntry(engine, classIndex);
        uint32_t instanceIndex = AspDataGetContextInstanceIndex(context);
        if (instanceIndex != 0)
            instance = AspValueEntry(engine, instanceIndex);
    }

    *returnValue = AspAllocEntry(engine, DataType_Super);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;
    AspRef(engine, cls);
    AspDataSetSuperClassIndex(*returnValue, AspIndex(engine, cls));
    AspRef(engine, instance);
    AspDataSetSuperInstanceIndex(*returnValue, AspIndex(engine, instance));

    return AspRunResult_OK;
}

/* len(object)
 * Return length of object. Return 1 if object is not like a container.
 */
ASP_LIB_API AspRunResult AspLib_len
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    AspRunResult result = AspRunResult_OK;

    int32_t count;
    result = AspCount(engine, object, &count);
    if (result != AspRunResult_OK)
        return result;

    return
        (*returnValue = AspNewInteger(engine, count)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
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

    return
        (*returnValue = AspNewBoolean(engine, AspIsTrue(engine, x))) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* int(x, base, check)
 * Convert a number or string to an integer.
 * Floats are truncated towards zero. Out of range floats (including infinities
 * and NaNs) raise an error condition if check is true. Otherwise, out of range
 * floats (including infinities) are converted to either INT32_MIN or INT32_MAX
 * according to the sign, and NaNs are converted to zero.
 * Strings are treated in the normal C way, as per strtol. Malformed strings
 * raise an error condition if check is true. Otherwise, None is returned.
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

    bool checkValue = AspIsTrue(engine, check);

    int32_t intValue;
    if (AspIsNumeric(x))
    {
        bool valid = AspIntegerValue(x, &intValue);
        if (!valid)
            return checkValue ? AspRunResult_ValueOutOfRange : AspRunResult_OK;
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
            return checkValue ? extractResult : AspRunResult_OK;

        /* Convert the word to an integer value. */
        char *endp;
        errno = 0;
        long longValue = strtol(buffer, &endp, (int)baseValue);
        if (*endp != '\0' || errno != 0 ||
            longValue < INT32_MIN || longValue > INT32_MAX)
            return checkValue ? AspRunResult_ValueOutOfRange : AspRunResult_OK;
        intValue = (int32_t)longValue;
    }
    else
        return AspRunResult_UnexpectedType;

    return
        (*returnValue = AspNewInteger(engine, intValue)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* float(x)
 * Convert a number or string to a float.
 * Strings are treated in the normal C way, as per strtod. If the string is
 * malformed, None is returned.
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
            return AspRunResult_OK;

        /* Convert the word to a floating-point value. */
        char *endp;
        errno = 0;
        floatValue = strtod(buffer, &endp);
        if (*endp != '\0' || errno != 0)
            return AspRunResult_OK;
    }
    else
        return AspRunResult_UnexpectedType;

    return
        (*returnValue = AspNewFloat(engine, floatValue)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* str(x)
 * Return a string representation of x.
 */
ASP_LIB_API AspRunResult AspLib_str
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return
        (*returnValue = AspToString(engine, x)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* repr(x)
 * Return the canonical string representation of x.
 */
ASP_LIB_API AspRunResult AspLib_repr
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return
        (*returnValue = AspToRepr(engine, x)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* range_values(x)
 * Return a tuple containing the range components.
 */
ASP_LIB_API AspRunResult AspLib_range_values
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    if (!AspIsRange(x))
        return AspRunResult_UnexpectedType;

    *returnValue = AspNewTuple(engine);
    if (*returnValue == 0)
        return AspRunResult_OutOfDataMemory;

    int32_t startValue, endValue, stepValue;
    AspGetRange(engine, x, &startValue, &endValue, &stepValue, 0);

    bool appendSuccess = true;

    bool startPresent = AspDataGetRangeHasStart(x);
    AspDataEntry *startEntry = startPresent ?
        AspValueEntry(engine, AspDataGetRangeStartIndex(x)) :
        AspNewInteger(engine, startValue);
    if (startEntry == 0)
        return AspRunResult_OutOfDataMemory;
    appendSuccess = AspTupleAppend
        (engine, *returnValue, startEntry, !startPresent);
    if (!appendSuccess)
        return AspRunResult_OutOfDataMemory;

    bool endPresent = AspDataGetRangeHasEnd(x);
    AspDataEntry *endEntry = endPresent ?
        AspValueEntry(engine, AspDataGetRangeEndIndex(x)) :
        AspNewNone(engine);
    if (endEntry == 0)
        return AspRunResult_OutOfDataMemory;
    appendSuccess = AspTupleAppend
        (engine, *returnValue, endEntry, !endPresent);
    if (!appendSuccess)
        return AspRunResult_OutOfDataMemory;

    bool stepPresent = AspDataGetRangeHasStep(x);
    AspDataEntry *stepEntry = stepPresent ?
        AspValueEntry(engine, AspDataGetRangeStepIndex(x)) :
        AspNewInteger(engine, stepValue);
    if (stepEntry == 0)
        return AspRunResult_OutOfDataMemory;
    appendSuccess = AspTupleAppend
        (engine, *returnValue, stepEntry, !stepPresent);
    if (!appendSuccess)
        return AspRunResult_OutOfDataMemory;

    return AspRunResult_OK;
}

/* key(object, stringize)
 * Convert a non-key to either a string, if requested, or None, both of which
 * are valid keys. If the object is already a key, it is returned as is.
 */
ASP_LIB_API AspRunResult AspLib_key
    (AspEngine *engine,
     AspDataEntry *object, AspDataEntry *stringize,
     AspDataEntry **returnValue)
{
    /* If the object is already a key, return it. */
    bool isImmutable;
    AspRunResult result = AspCheckIsImmutableObject
        (engine, object, &isImmutable);
    if (result != AspRunResult_OK)
        return result;
    if (isImmutable)
    {
        AspRef(engine, object);
        *returnValue = object;
        return AspRunResult_OK;
    }

    /* Convert non-keys to a string if requested. */
    if (AspIsTrue(engine, stringize))
    {
        return
            (*returnValue = AspToString(engine, object)) == 0 ?
            AspRunResult_OutOfDataMemory : AspRunResult_OK;
    }

    return AspRunResult_OK;
}

static AspRunResult ExtractWord
    (AspEngine *engine, AspDataEntry *str,
     char *buffer, size_t *bufferSize)
{
    AspRunResult result = AspRunResult_OK;

    size_t maxBufferSize = *bufferSize;
    *bufferSize = 0;
    bool end = false;
    int32_t length;
    result = AspCount(engine, str, &length);
    if (result != AspRunResult_OK)
        return result;
    for (int32_t index = 0; index < length; index++)
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
