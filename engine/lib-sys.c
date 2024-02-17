/*
 * Asp script function library implementation: system functions.
 */

#include "asp-priv.h"

/* exit(code)
 * Exit the script with an appropriate result.
 * If code is None or integer zero, exit normally.
 * For non-zero integers, use as an offset from AspRunResult_Application.
 * For all other values, just use AspRunResult_Application.
 */
ASP_LIB_API AspRunResult AspLib_exit
    (AspEngine *engine,
     AspDataEntry *code,
     AspDataEntry **returnValue)
{
    int32_t intCode = 0;
    bool isInt = AspIntegerValue(code, &intCode);
    engine->runResult = isInt && intCode == 0 || AspIsNone(code) ?
        AspRunResult_Complete :
        (AspRunResult)((int)AspRunResult_Application + intCode);
    return AspRunResult_OK;
}
