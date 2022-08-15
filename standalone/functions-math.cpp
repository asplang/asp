//
// Standalone Asp application functions implementation: math.
//

#include "asp.h"
#include <cmath>

static AspRunResult generic_function
    (AspEngine *engine,
     double (*function)(double),
     AspDataEntry *a,
     AspDataEntry **returnValue);
static AspRunResult generic_function
    (AspEngine *engine,
     double (*function)(double, double),
     AspDataEntry *a, AspDataEntry *b,
     AspDataEntry **returnValue);

extern "C" AspRunResult asp_cos
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, cos, x, returnValue);
}

extern "C" AspRunResult asp_sin
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, sin, x, returnValue);
}

extern "C" AspRunResult asp_tan
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, tan, x, returnValue);
}

extern "C" AspRunResult asp_acos
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, acos, x, returnValue);
}

extern "C" AspRunResult asp_asin
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, asin, x, returnValue);
}

extern "C" AspRunResult asp_atan
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, atan, x, returnValue);
}

extern "C" AspRunResult asp_atan2
    (AspEngine *engine,
     AspDataEntry *y, AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, atan2, y, x, returnValue);
}

extern "C" AspRunResult asp_cosh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, cosh, x, returnValue);
}

extern "C" AspRunResult asp_sinh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, sinh, x, returnValue);
}

extern "C" AspRunResult asp_tanh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, tanh, x, returnValue);
}

extern "C" AspRunResult asp_acosh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, acosh, x, returnValue);
}

extern "C" AspRunResult asp_asinh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, asinh, x, returnValue);
}

extern "C" AspRunResult asp_atanh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, atanh, x, returnValue);
}

extern "C" AspRunResult asp_hypot
    (AspEngine *engine,
     AspDataEntry *x, AspDataEntry *y,
     AspDataEntry **returnValue)
{
    return generic_function(engine, hypot, x, y, returnValue);
}

extern "C" AspRunResult asp_exp
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, exp, x, returnValue);
}

extern "C" AspRunResult asp_log
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, log, x, returnValue);
}

extern "C" AspRunResult asp_log10
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, log10, x, returnValue);
}

extern "C" AspRunResult asp_ceil
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, ceil, x, returnValue);
}

extern "C" AspRunResult asp_floor
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, floor, x, returnValue);
}

extern "C" AspRunResult asp_round
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, round, x, returnValue);
}

extern "C" AspRunResult asp_abs
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function(engine, fabs, x, returnValue);
}

static AspRunResult generic_function
    (AspEngine *engine,
     double (*function)(double),
     AspDataEntry *a,
     AspDataEntry **returnValue)
{
    double aValue;
    if (!AspFloatValue(a, &aValue))
        return AspRunResult_UnexpectedType;
    double resultValue = function(aValue);
    *returnValue = AspNewFloat(engine, resultValue);
    return AspAssert(engine, *returnValue != 0);
}

static AspRunResult generic_function
    (AspEngine *engine,
     double (*function)(double, double),
     AspDataEntry *a, AspDataEntry *b,
     AspDataEntry **returnValue)
{
    double aValue, bValue;
    if (!AspFloatValue(a, &aValue))
        return AspRunResult_UnexpectedType;
    if (!AspFloatValue(b, &bValue))
        return AspRunResult_UnexpectedType;
    double resultValue = function(aValue, bValue);
    *returnValue = AspNewFloat(engine, resultValue);
    return AspAssert(engine, *returnValue != 0);
}
