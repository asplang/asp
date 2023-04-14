//
// Standalone Asp application functions implementation: print.
//

#include "asp.h"
#include "standalone.h"
#include <stdio.h>

static AspRunResult asp_print1(AspEngine *, AspDataEntry *);

/* print(*values)
 * Print value to standard output.
 * Separate individual items with a space.
 *
 * TODO: Add spacing parameter to support separators other than space.
 */
extern "C" AspRunResult asp_print
    (AspEngine *engine,
     AspDataEntry *values, /* group */
     AspDataEntry **returnValue)
{
    unsigned argCount = AspCount(values);
    for (unsigned i = 0; i < argCount; i++)
    {
        if (i != 0)
            putchar(' ');

        AspDataEntry *value = AspElement(engine, values, i);
        AspRunResult result = asp_print1(engine, value);
        if (result != AspRunResult_OK)
            return result;
    }

    puts("");
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
        AspStringValue(engine, valueString, 0, buffer, index, sizeof buffer);
        printf("%.*s", (int)(sizeof buffer), buffer);
    }

    AspUnref(engine, valueString);
    return AspRunResult_OK;
}
