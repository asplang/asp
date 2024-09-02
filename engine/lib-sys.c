/*
 * Asp script function library implementation: system functions.
 */

#include "asp-priv.h"
#include "tree.h"
#include "data.h"
#include <stdbool.h>
#include <stdint.h>

/* module()
 * Return the current module.
 */
ASP_LIB_API AspRunResult AspLib_module
    (AspEngine *engine,
     AspDataEntry **returnValue)
{
    AspRef(engine, engine->module);
    *returnValue = engine->module;
    return AspRunResult_OK;
}

/* exists(symbol)
 * Return True if the symbol can be found in any of the local, module, or
 * system scopes. Return None if argument is not a symbol.
 */
ASP_LIB_API AspRunResult AspLib_exists
    (AspEngine *engine,
     AspDataEntry *symbol,
     AspDataEntry **returnValue)
{
    int32_t symbolValue;
    if (!AspSymbolValue(symbol, &symbolValue))
        return AspRunResult_OK;

    AspDataEntry *namespaces[] =
    {
        engine->localNamespace,
        engine->globalNamespace,
        engine->systemNamespace,
    };
    bool found = false;
    for (size_t i = namespaces[1] == namespaces[0] ? 1 : 0;
         i < sizeof namespaces / sizeof *namespaces; i++)
    {
        AspTreeResult result = AspFindSymbol
            (engine, namespaces[i], symbolValue);
        if (result.result != AspRunResult_OK)
            return result.result;
        if (result.value != 0)
        {
            found = true;
            break;
        }
    }

    return
        (*returnValue = AspNewBoolean(engine, found)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

/* id(object)
 * Return the object's identity. This is the object's data entry index,
 * converted to a signed integer.
 */
ASP_LIB_API AspRunResult AspLib_id
    (AspEngine *engine,
     AspDataEntry *object,
     AspDataEntry **returnValue)
{
    uint32_t index = AspIndex(engine, object);
    int32_t id = *(int32_t *)&index;
    return
        (*returnValue = AspNewInteger(engine, id)) == 0 ?
        AspRunResult_OutOfDataMemory : AspRunResult_OK;
}

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
    bool isInt = AspIsInteger(code);
    if (isInt)
    {
        AspIntegerValue(code, &intCode);
        const int32_t maxIntCode =
            (int32_t)AspRunResult_Max - (int32_t)AspRunResult_Application;
        if (intCode < 0 || intCode > maxIntCode)
            intCode = maxIntCode;
    }
    engine->runResult = isInt && intCode == 0 || AspIsNone(code) ?
        AspRunResult_Complete :
        (AspRunResult)((int)AspRunResult_Application + intCode);
    return AspRunResult_OK;
}
