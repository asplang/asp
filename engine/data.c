/*
 * Asp engine data implementation.
 */

#include "data.h"
#include "stack.h"
#include "sequence.h"
#include "asp-priv.h"
#include <string.h>

static bool IsSimpleImmutableObject(const AspDataEntry *);

void AspDataSetWord3(AspDataEntry *entry, uint32_t value)
{
    entry->s[11] = (uint8_t)AspBitGetField(value, 0, 8);
    entry->s[12] = (uint8_t)AspBitGetField(value, 8, 8);
    entry->s[13] = (uint8_t)AspBitGetField(value, 16, 8);
    AspBitSetField
        (&entry->u2, AspWordBitSize, AspWordBitSize - 24U,
         AspBitGetField(value, 24U, AspWordBitSize - 24U));
}

uint32_t AspDataGetWord3(const AspDataEntry *entry)
{
    return (uint32_t)
        (uint32_t)entry->s[11] |
        (uint32_t)entry->s[12] << 8 |
        (uint32_t)entry->s[13] << 16 |
        AspBitGetField(entry->u2, AspWordBitSize, AspWordBitSize - 24U) << 24;
}

size_t AspDataEntrySize(void)
{
    return sizeof(AspDataEntry);
}

void AspClearData(AspEngine *engine)
{
    /* Clear data storage, setting every element to a free entry. */
    engine->freeListIndex = 0;
    AspDataEntry *data = engine->data;
    for (unsigned i = 0; i < engine->dataEndIndex; i++)
    {
        memset(data + i, 0, sizeof *data);
        AspDataSetType(data + i, DataType_Free);
        AspDataSetFreeNext(data + i, i + 1);
    }
    if (engine->dataEndIndex != 0)
        AspDataSetFreeNext(data + engine->dataEndIndex - 1, 0);
    engine->lowFreeCount = engine->freeCount = engine->dataEndIndex;
}

uint32_t AspAlloc(AspEngine *engine)
{
    if (engine->freeCount == 0)
    {
        engine->runResult = AspRunResult_OutOfDataMemory;
        return 0;
    }

    AspDataEntry *data = engine->data;
    uint32_t index = engine->freeListIndex;

    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(data + index) == DataType_Free);
    if (assertResult != AspRunResult_OK)
        return 0;

    engine->freeListIndex = AspDataGetFreeNext(data + index);
    engine->freeCount--;
    if (engine->freeCount < engine->lowFreeCount)
        engine->lowFreeCount = engine->freeCount;
    memset(data + index, 0, sizeof *data);
    AspDataSetType(data + index, DataType_None);
    return index;
}

bool AspFree(AspEngine *engine, uint32_t index)
{
    AspRunResult assertResult = AspAssert
        (engine, index < engine->dataEndIndex);
    if (assertResult != AspRunResult_OK)
        return false;
    AspDataEntry *data = engine->data;
    assertResult = AspAssert
        (engine, AspDataGetType(data + index) != DataType_Free);
    if (assertResult != AspRunResult_OK)
        return false;

    memset(data + index, 0, sizeof *data);
    AspDataSetType(data + index, DataType_Free);
    AspDataSetFreeNext(data + index, engine->freeListIndex);
    engine->freeListIndex = index;
    engine->freeCount++;

    return true;
}

bool AspIsObject(const AspDataEntry *entry)
{
    return (AspDataGetType(entry) & ~DataType_ObjectMask) == 0;
}

bool AspIsImmutableObject(AspEngine *engine, const AspDataEntry *entry)
{
    /* Perform a simple check if possible. */
    if (AspDataGetType(entry) != DataType_Tuple)
        return IsSimpleImmutableObject(entry);

    /* For tuples, we must examine the contents. Avoid recursion by using
       the engine's stack. */
    bool isImmutable = true;
    AspDataEntry *startStackTop = engine->stackTop;
    while (true)
    {
        for (AspSequenceResult nextResult = AspSequenceNext
                (engine, (AspDataEntry *)entry, 0);
             nextResult.element != 0;
             nextResult = AspSequenceNext
                (engine, (AspDataEntry *)entry, nextResult.element))
        {
            AspDataEntry *value = nextResult.value;

            if (AspDataGetType(value) == DataType_Tuple)
                AspPushNoUse(engine, nextResult.value);
            else if (!IsSimpleImmutableObject(value))
            {
                isImmutable = false;
                break;
            }
        }
        if (!isImmutable)
            break;

        /* Check if there's more to do. */
        if (engine->stackTop == startStackTop ||
            engine->runResult != AspRunResult_OK)
            break;

        entry = AspTop(engine);
        AspPopNoErase(engine);
    }

    /* Unwind the working stack if necessary. */
    if (engine->runResult == AspRunResult_OK)
        while (engine->stackTop != startStackTop)
            AspPopNoErase(engine);

    return isImmutable;
}

static bool IsSimpleImmutableObject(const AspDataEntry *entry)
{
    uint8_t type = AspDataGetType(entry);
    return
        AspIsObject(entry) &&
        type != DataType_Tuple &&
        type != DataType_List &&
        type != DataType_Set &&
        type != DataType_Dictionary &&
        type != DataType_Iterator;
}

AspDataEntry *AspAllocEntry(AspEngine *engine, DataType type)
{
    /* Return the None singleton if the None type is requested. */
    if (type == DataType_None)
    {
        AspRef(engine, engine->data);
        return engine->data;
    }

    uint32_t index = AspAlloc(engine);
    if (index == 0)
        return 0;
    AspDataEntry *entry = engine->data + index;
    AspDataSetType(entry, type);
    AspRef(engine, entry);
    return entry;
}

AspDataEntry *AspEntry(AspEngine *engine, uint32_t index)
{
    /* Zero means a null reference (for lists, trees, etc.). */
    return index == 0 ? 0 : engine->data + index;
}

AspDataEntry *AspValueEntry(AspEngine *engine, uint32_t index)
{
    /* Zero means the None singleton. */
    return engine->data + index;
}

uint32_t AspIndex(const AspEngine *engine, const AspDataEntry *entry)
{
    return entry == 0 ? 0 : (uint32_t)(entry - engine->data);
}
