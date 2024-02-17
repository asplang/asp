//
// Standalone Asp application functions implementation: print.
//

#include "asp.h"
#include "standalone.h"
#include <stdio.h>

static AspRunResult asp_print1(AspEngine *, AspDataEntry *);

/* print(*values, sep, end)
 * Print values to standard output.
 * Separate individual items with the sep value and finish with the end value.
 */
extern "C" AspRunResult asp_print
    (AspEngine *engine,
     AspDataEntry *values, /* tuple group */
     AspDataEntry *sep, AspDataEntry *end,
     AspDataEntry **returnValue)
{
    unsigned argCount = AspCount(values);
    for (unsigned i = 0; i < argCount; i++)
    {
        if (i != 0)
        {
            AspRunResult result = asp_print1(engine, sep);
            if (result != AspRunResult_OK)
                return result;
        }

        AspDataEntry *value = AspElement(engine, values, i);
        AspRunResult result = asp_print1(engine, value);
        if (result != AspRunResult_OK)
            return result;
    }

    AspRunResult result = asp_print1(engine, end);
    if (result != AspRunResult_OK)
        return result;

    return AspRunResult_OK;
}

static AspRunResult asp_print1
    (AspEngine *engine, AspDataEntry *value)
{
    AspDataEntry *valueString = AspToString(engine, value);
    if (valueString == 0)
        return AspRunResult_OutOfDataMemory;

    size_t size;
    AspStringValue(engine, valueString, &size, 0, 0, 0);
    char buffer[16];
    for (size_t index = 0; index < size; index += sizeof buffer)
    {
        size_t bufferLen = sizeof buffer;
        if (index + sizeof buffer > size)
            bufferLen = size - index;
        AspStringValue(engine, valueString, 0, buffer, index, sizeof buffer);
        for (size_t byteIndex = 0; byteIndex < bufferLen; byteIndex++)
            putchar(buffer[byteIndex]);
    }

    AspUnref(engine, valueString);
    return AspRunResult_OK;
}
