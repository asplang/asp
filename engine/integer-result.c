/*
 * Asp engine integer arithmetic result implementation.
 */

#include "integer-result.h"

AspRunResult AspTranslateIntegerResult(AspIntegerResult result)
{
    switch(result)
    {
        default:
            return AspRunResult_InternalError;
        case AspIntegerResult_OK:
            return AspRunResult_OK;
        case AspIntegerResult_ValueOutOfRange:
            return AspRunResult_ValueOutOfRange;
        case AspIntegerResult_DivideByZero:
            return AspRunResult_DivideByZero;
        case AspIntegerResult_ArithmeticOverflow:
            return AspRunResult_ArithmeticOverflow;
    }
}
