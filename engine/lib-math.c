/*
 * Asp script function library implementation: math.
 */

#include "asp.h"
#include <math.h>

static AspRunResult generic_function_1
    (AspEngine *engine,
     double (*function)(double),
     AspDataEntry *a,
     AspDataEntry **returnValue);
static AspRunResult generic_function_2
    (AspEngine *engine,
     double (*function)(double, double),
     AspDataEntry *a, AspDataEntry *b,
     AspDataEntry **returnValue);

/* sin(x)
 * Return the sine of x radians.
 */
AspRunResult AspLib_sin
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, sin, x, returnValue);
}

/* cos(x)
 * Return the cosine of x radians.
 */
AspRunResult AspLib_cos
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, cos, x, returnValue);
}

/* tan(x)
 * Return the tangent of x radians.
 */
AspRunResult AspLib_tan
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, tan, x, returnValue);
}

/* asin(x)
 * Return the arc sine, in radians, of x.
 */
AspRunResult AspLib_asin
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, asin, x, returnValue);
}

/* acos(x)
 * Return the arc cosine, in radians, of x.
 */
AspRunResult AspLib_acos
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, acos, x, returnValue);
}

/* atan(x)
 * Return the arc tangent, in radians, of x.
 */
AspRunResult AspLib_atan
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, atan, x, returnValue);
}

/* atan2(y, x)
 * Return the arc tangent, in radians, of y/x.
 */
AspRunResult AspLib_atan2
    (AspEngine *engine,
     AspDataEntry *y, AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_2(engine, atan2, y, x, returnValue);
}

/* sinh(x)
 * Return the hyperbolic sine of x.
 */
AspRunResult AspLib_sinh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, sinh, x, returnValue);
}

/* cosh(x)
 * Return the hyperbolic cosine of x.
 */
AspRunResult AspLib_cosh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, cosh, x, returnValue);
}

/* tanh(x)
 * Return the hyperbolic tangent of x.
 */
AspRunResult AspLib_tanh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, tanh, x, returnValue);
}

/* asinh(x)
 * Return the inverse hyperbolic sine of x.
 */
AspRunResult AspLib_asinh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, asinh, x, returnValue);
}

/* acosh(x)
 * Return the inverse hyperbolic cosine of x.
 */
AspRunResult AspLib_acosh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, acosh, x, returnValue);
}

/* atanh(x)
 * Return the inverse hyperbolic tangent of x.
 */
AspRunResult AspLib_atanh
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, atanh, x, returnValue);
}

/* hypot(x, y)
 * Return the Euclidean distance, sqrt(x*x + y*y).
 * This is the hypotenuse of a right-angled triangle whose legs are x and y.
 */
AspRunResult AspLib_hypot
    (AspEngine *engine,
     AspDataEntry *x, AspDataEntry *y,
     AspDataEntry **returnValue)
{
    return generic_function_2(engine, hypot, x, y, returnValue);
}

/* exp(x)
 * Return e raised to the power of x.
 */
AspRunResult AspLib_exp
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, exp, x, returnValue);
}

/* log(x)
 * Return the natural (base e) logarithm of x.
 *
 * TODO: Add base parameter to support arbitrary bases.
 */
AspRunResult AspLib_log
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, log, x, returnValue);
}

/* log10(x)
 * Return the base 10 logarithm of x.
 */
AspRunResult AspLib_log10
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, log10, x, returnValue);
}

/* ceil(x)
 * Return the ceiling of x as a float.
 * This is the smallest integral value >= x.
 */
AspRunResult AspLib_ceil
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, ceil, x, returnValue);
}

/* floor(x)
 * Return the floor of x as a float.
 * This is the largest integral value <= x.
 */
AspRunResult AspLib_floor
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, floor, x, returnValue);
}

/* round(x)
 * Return the integral value that is nearest to x, with halfway cases rounded
 * away from zero.
 */
AspRunResult AspLib_round
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, round, x, returnValue);
}

/* abs(x)
 * Return the absolute value of x.
 */
AspRunResult AspLib_abs
    (AspEngine *engine,
     AspDataEntry *x,
     AspDataEntry **returnValue)
{
    return generic_function_1(engine, fabs, x, returnValue);
}

static AspRunResult generic_function_1
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

static AspRunResult generic_function_2
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
