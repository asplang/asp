/*
 * Asp engine stack implementation.
 */

#include "stack.h"
#include "asp-priv.h"
#include "data.h"

static AspDataEntry *AspPush1(AspEngine *, AspDataEntry *, bool use);
static bool AspPop1(AspEngine *, bool eraseValue);

AspDataEntry *AspPush(AspEngine *engine, AspDataEntry *value)
{
    return AspPush1(engine, value, true);
}

AspDataEntry *AspPushNoUse(AspEngine *engine, AspDataEntry *value)
{
    return AspPush1(engine, value, false);
}

static AspDataEntry *AspPush1(AspEngine *engine, AspDataEntry *value, bool use)
{
    AspAssert(engine, value != 0);
    AspRunResult assertResult = AspAssert
        (engine, AspDataGetType(value) != DataType_Free);
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
    return AspPop1(engine, true);
}

bool AspPopNoErase(AspEngine *engine)
{
    return AspPop1(engine, false);
}

static bool AspPop1(AspEngine *engine, bool eraseValue)
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
