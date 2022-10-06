/*
 * Asp engine assignment implementation.
 */

#include "assign.h"
#include "stack.h"
#include "sequence.h"

AspRunResult AspAssignSimple
    (AspEngine *engine,
     AspDataEntry *address, AspDataEntry *newValue)
{
    uint8_t addressType = AspDataGetType(address);

    AspRunResult assertResult = AspAssert
        (engine,
         addressType == DataType_Element ||
         addressType == DataType_DictionaryNode ||
         addressType == DataType_NamespaceNode);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    uint32_t newValueIndex = AspIndex(engine, newValue);
    switch (addressType)
    {
        case DataType_Element:
        {
            AspDataEntry *oldValue = AspValueEntry
                (engine, AspDataGetElementValueIndex(address));
            AspUnref(engine, oldValue);
            AspDataSetElementValueIndex(address, newValueIndex);
            break;
        }

        case DataType_DictionaryNode:
        case DataType_NamespaceNode:
        {
            AspDataEntry *oldValue = AspValueEntry
                (engine, AspDataGetTreeNodeValueIndex(address));
            AspUnref(engine, oldValue);
            AspDataSetTreeNodeValueIndex(address, newValueIndex);
            break;
        }
    }

    AspRef(engine, newValue);
    return AspRunResult_OK;
}

AspRunResult AspAssignTuple
    (AspEngine *engine,
     AspDataEntry *address, AspDataEntry *newValue)
{
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(address) == DataType_Tuple);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    AspRunResult checkResult = AspCheckTupleMatch(engine, address, newValue);
    if (checkResult != AspRunResult_OK)
        return checkResult;

    AspDataEntry *startStackTop = engine->stackTop;

    /* Avoid recursion by using the engine's stack. */
    for (bool unrefNewValue = false; ; unrefNewValue = true)
    {
        AspSequenceResult newValueIterResult = {AspRunResult_OK, 0, 0};
        for (AspSequenceResult addressIterResult = AspSequenceNext
                (engine, address, 0);
             addressIterResult.element != 0;
             addressIterResult = AspSequenceNext
                (engine, address, addressIterResult.element))
        {
            AspDataEntry *addressElement = addressIterResult.value;

            newValueIterResult = AspSequenceNext
                (engine, newValue, newValueIterResult.element);
            AspDataEntry *newValueElement = newValueIterResult.value;

            if (AspDataGetType(addressElement) == DataType_Tuple)
            {
                AspRunResult checkResult = AspCheckTupleMatch
                    (engine, addressElement, newValueElement);
                if (checkResult != AspRunResult_OK)
                    return checkResult;

                AspDataEntry *stackEntry = AspPush(engine, newValueElement);
                if (stackEntry == 0)
                    return AspRunResult_OutOfDataMemory;
                stackEntry = AspPush(engine, addressElement);
                if (stackEntry == 0)
                    return AspRunResult_OutOfDataMemory;
            }
            else
            {
                AspRunResult assignResult = AspAssignSimple
                    (engine, addressElement, newValueElement);
                if (assignResult != AspRunResult_OK)
                    return assignResult;
            }
        }

        /* Make sure not to unreference the top-level value, as the
           type of instruction (SET or SETP) decides how it should be
           handled. */
        AspUnref(engine, address);
        if (unrefNewValue)
            AspUnref(engine, newValue);

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        address = AspTop(engine);
        if (address == 0)
            return AspRunResult_StackUnderflow;
        AspRef(engine, address);
        AspPop(engine);
        newValue = AspTop(engine);
        if (newValue == 0)
            return AspRunResult_StackUnderflow;
        AspRef(engine, newValue);
        AspPop(engine);
    }

    return AspRunResult_OK;
}

AspRunResult AspCheckTupleMatch
    (AspEngine *engine,
     const AspDataEntry *address, const AspDataEntry *value)
{
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(address) == DataType_Tuple);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    if (AspDataGetType(value) != DataType_Tuple)
        return AspRunResult_UnexpectedType;
    uint32_t addressCount = AspDataGetSequenceCount(address);
    uint32_t valueCount = AspDataGetSequenceCount(value);
    return
        addressCount != valueCount ?
        AspRunResult_TupleMismatch : AspRunResult_OK;
}
