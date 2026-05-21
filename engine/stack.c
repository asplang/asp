/*
 * Asp engine stack implementation.
 */

#include "stack.h"
#include "asp-priv.h"
#include "data.h"

static AspDataEntry *Push(AspEngine *, AspDataEntry *, bool use);
static bool Pop(AspEngine *, bool eraseValue);

AspDataEntry *AspPush(AspEngine *engine, AspDataEntry *value)
{
    return Push(engine, value, true);
}

AspDataEntry *AspPushNoUse(AspEngine *engine, const AspDataEntry *value)
{
    return Push(engine, (AspDataEntry *)value, false);
}

static AspDataEntry *Push(AspEngine *engine, AspDataEntry *value, bool use)
{
    AspRunResult assertResult = AspAssert
        (engine, value != 0 && AspDataGetType(value) != DataType_Free);
    if (assertResult != AspRunResult_OK)
        return 0;

    AspDataEntry *newTopEntry = AspAllocEntry(engine, DataType_StackEntry);
    if (newTopEntry == 0)
        return 0;
    AspDataSetStackEntryPreviousIndex(newTopEntry,
        engine->stackTop == 0 ? 0 : AspIndex(engine, engine->stackTop));
    AspDataSetStackEntryValueIndex(newTopEntry, AspIndex(engine, value));
    if (use)
        AspRef(engine, value);
    engine->stackTop = newTopEntry;
    engine->stackCount++;

    return newTopEntry;
}

AspDataEntry *AspTopValue(AspEngine *engine)
{
    if (engine->stackTop == 0)
        return 0;
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(engine->stackTop) == DataType_StackEntry);
    if (assertResult != AspRunResult_OK)
        return 0;

    AspDataEntry *value = AspValueEntry
        (engine, AspDataGetStackEntryValueIndex(engine->stackTop));
    assertResult = AspAssert
        (engine, AspDataGetType(value) != DataType_Free);
    if (assertResult != AspRunResult_OK)
        return 0;

    return value;
}

AspDataEntry *AspTopValue2(AspEngine *engine)
{
    if (engine->stackTop == 0)
        return 0;
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(engine->stackTop) == DataType_StackEntry);
    if (assertResult != AspRunResult_OK)
        return 0;
    if (!AspDataGetStackEntryHasValue2(engine->stackTop))
        return 0;

    AspDataEntry *value = AspValueEntry
        (engine, AspDataGetStackEntryValue2Index(engine->stackTop));
    assertResult = AspAssert
        (engine, AspDataGetType(engine->stackTop) != DataType_Free);
    if (assertResult != AspRunResult_OK)
        return 0;

    return value;
}

bool AspPop(AspEngine *engine)
{
    return Pop(engine, true);
}

bool AspPopNoErase(AspEngine *engine)
{
    return Pop(engine, false);
}

static bool Pop(AspEngine *engine, bool eraseValue)
{
    if (engine->stackTop == 0)
        return false;
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(engine->stackTop) == DataType_StackEntry);
    if (assertResult != AspRunResult_OK)
        return false;

    AspDataEntry *value = AspTopValue(engine);
    if (value == 0)
        return false;
    if (eraseValue && AspIsObject(value))
        AspUnref(engine, value);

    uint32_t prevIndex = AspDataGetStackEntryPreviousIndex(engine->stackTop);
    AspUnref(engine, engine->stackTop);
    engine->stackTop = prevIndex == 0 ? 0 : AspEntry(engine, prevIndex);
    engine->stackCount--;

    return true;
}
