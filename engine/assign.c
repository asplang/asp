/*
 * Asp engine assignment implementation.
 */

#include "assign.h"
#include "stack.h"
#include "sequence.h"


static AspRunResult AspCheckSequenceMatch
    (AspEngine *, const AspDataEntry *address, const AspDataEntry *newValue);

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

AspRunResult AspAssignSequence
    (AspEngine *engine,
     AspDataEntry *address, AspDataEntry *newValue)
{
    DataType addressType = AspDataGetType(address);
    AspRunResult assertResult = AspAssert
        (engine,
         addressType == DataType_Tuple || addressType == DataType_List);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    AspRunResult checkResult = AspCheckSequenceMatch
        (engine, address, newValue);
    if (checkResult != AspRunResult_OK)
        return checkResult;

    /* Avoid recursion by using the engine's stack. */
    AspDataEntry *startStackTop = engine->stackTop;
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

            DataType addressElementType = AspDataGetType(addressElement);
            if (addressElementType == DataType_Tuple ||
                addressElementType == DataType_List)
            {
                AspRunResult checkResult = AspCheckSequenceMatch
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

AspRunResult AspCheckSequenceMatch
    (AspEngine *engine,
     const AspDataEntry *address, const AspDataEntry *value)
{
    DataType addressType = AspDataGetType(address);
    AspRunResult assertResult = AspAssert
        (engine,
         addressType == DataType_Tuple || addressType == DataType_List);
    if (assertResult != AspRunResult_OK)
        return assertResult;

    DataType valueType = AspDataGetType(value);
    if (valueType != DataType_Tuple && valueType != DataType_List)
        return AspRunResult_UnexpectedType;
    uint32_t addressCount = AspDataGetSequenceCount(address);
    uint32_t valueCount = AspDataGetSequenceCount(value);
    return
        addressCount != valueCount ?
        AspRunResult_SequenceMismatch : AspRunResult_OK;
}
