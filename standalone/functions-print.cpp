//
// Standalone Asp application functions implementation: print.
//

#include "asp.h"
#include <stdio.h>
#include <limits.h>

static AspRunResult asp_print1
    (AspEngine *engine,
     AspDataEntry *value,
     AspDataEntry **returnValue);

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

        AspDataEntry *value = AspListElement(engine, values, i);
        AspRunResult result = asp_print1(engine, value, returnValue);
        if (result != AspRunResult_OK)
            return result;
    }

    puts("");
    return AspRunResult_OK;
}

static AspRunResult asp_print1
    (AspEngine *engine,
     AspDataEntry *value,
     AspDataEntry **returnValue)
{
    if (AspIsNone(value))
        printf("None");
    else if (AspIsEllipsis(value))
        printf("...");
    else if (AspIsBoolean(value))
        printf("%s", AspIsTrue(engine, value) ? "True" : "False");
    else if (AspIsInteger(value))
    {
        int32_t i;
        AspIntegerValue(value, &i);
        printf("%d", i);
    }
    else if (AspIsFloat(value))
    {
        double f;
        AspFloatValue(value, &f);
        printf("%g", f);
    }
    else if (AspIsRange(value))
    {
        int32_t start, end, step;
        AspRangeValue(engine, value, &start, &end, &step);
        if (start != 0)
            printf("%d", start);
        printf("..");
        bool unbounded =
            step < 0 && end == INT32_MIN ||
            step > 0 && end == INT32_MAX;
        if (!unbounded)
            printf("%d", end);
        if (step != 1)
            printf(":%d", step);
    }
    else if (AspIsString(value))
    {
        size_t size;
        AspStringValue(engine, value, &size, 0, 0, 0);
        char buffer[16];
        for (size_t index = 0; index < size; index += sizeof buffer)
        {
            AspStringValue(engine, value, 0, buffer, index, sizeof buffer);
            printf("%.*s", (int)(sizeof buffer), buffer);
        }
    }
    else
        printf("<unsupported type>");

    return AspRunResult_OK;
}
