/*
 * Asp engine sequence implementation.
 */

#include "sequence.h"
#include "data.h"

static bool IsSequenceType(DataType);

AspSequenceResult AspSequenceAppend
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *value)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert(engine, list != 0 && IsSequenceType(AspDataGetType(list)));
    result.result = AspAssert(engine, value != 0);
    if (result.result != AspRunResult_OK)
        return result;

    /* Allocate an element entry and link it to the given value entry. */
    result.element = AspAllocEntry(engine, DataType_Element);
    if (result.element == 0)
    {
        result.result = AspRunResult_OutOfDataMemory;
        return result;
    }
    result.value = value;
    AspRef(engine, value);
    AspDataSetElementValueIndex(result.element, AspIndex(engine, value));

    /* Update links. */
    uint32_t elementIndex = AspIndex(engine, result.element);
    if (AspDataGetSequenceHeadIndex(list) == 0)
    {
        AspDataSetSequenceHeadIndex(list, elementIndex);
        AspDataSetSequenceTailIndex(list, elementIndex);
    }
    else
    {
        uint32_t tailIndex = AspDataGetSequenceTailIndex(list);
        AspDataEntry *tail = AspEntry(engine, tailIndex);
        AspDataSetElementNextIndex(tail, elementIndex);
        AspDataSetElementPreviousIndex(result.element, tailIndex);
        AspDataSetSequenceTailIndex(list, AspIndex(engine, result.element));
    }

    /* Update the sequence count. */
    uint32_t sizeChange = AspDataGetType(list) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;
    AspDataSetSequenceCount(list, AspDataGetSequenceCount(list) + sizeChange);

    return result;
}

AspSequenceResult AspSequenceInsert
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *value,
     AspDataEntry *element)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert(engine, list != 0 && IsSequenceType(AspDataGetType(list)));
    result.result = AspAssert(engine, value != 0);
    if (result.result != AspRunResult_OK)
        return result;

    /* TODO: Implement. */
    result.result = AspRunResult_NotImplemented;

    return result;
}

bool AspSequenceErase
    (AspEngine *engine, AspDataEntry *list, unsigned index,
     bool eraseValue)
{
    AspSequenceResult result = AspSequenceIndex(engine, list, index);
    if (result.element == 0)
        return false;

    return AspSequenceEraseElement(engine, list, result.element, eraseValue);
}

bool AspSequenceEraseElement
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *element,
     bool eraseValue)
{
    /* Update links in adjacent elements. */
    uint32_t prevIndex = AspDataGetElementPreviousIndex(element);
    uint32_t nextIndex = AspDataGetElementNextIndex(element);
    if (prevIndex == 0)
        AspDataSetSequenceHeadIndex(list, nextIndex);
    else
        AspDataSetElementNextIndex(AspEntry(engine, prevIndex), nextIndex);
    if (nextIndex == 0)
        AspDataSetSequenceTailIndex(list, prevIndex);
    else
        AspDataSetElementPreviousIndex(AspEntry(engine, nextIndex), prevIndex);

    /* Prepare to drop the element. */
    AspDataEntry *value = AspValueEntry
        (engine, AspDataGetElementValueIndex(element));
    uint32_t sizeChange = AspDataGetType(list) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;

    /* Unreference entries. */
    if (eraseValue)
        AspUnref(engine, value);
    AspUnref(engine, element);

    /* Update the sequence count. */
    AspDataSetSequenceCount(list, AspDataGetSequenceCount(list) - sizeChange);

    return true;
}

AspSequenceResult AspSequenceIndex
    (AspEngine *engine, AspDataEntry *list, unsigned index)
{
    AspSequenceResult result = AspSequenceNext(engine, list, 0);
    unsigned i = 0;
    for (i = 0; i < index && result.element != 0; i++)
        result = AspSequenceNext(engine, list, result.element);

    return result;
}

AspSequenceResult AspSequenceNext
    (AspEngine *engine, AspDataEntry *list, AspDataEntry *element)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine, list != 0 && IsSequenceType(AspDataGetType(list)));
    result.result = AspAssert
        (engine, element == 0 || AspDataGetType(element) == DataType_Element);
    if (result.result != AspRunResult_OK)
        return result;

    result.element = AspEntry
        (engine,
         element == 0 ?
         AspDataGetSequenceHeadIndex(list) :
         AspDataGetElementNextIndex(element));

    result.value = result.element == 0 ? 0 :
        AspEntry(engine, AspDataGetElementValueIndex(result.element));

    return result;
}

static bool IsSequenceType(DataType type)
{
    return
        type == DataType_String ||
        type == DataType_Tuple ||
        type == DataType_List ||
        type == DataType_ParameterList ||
        type == DataType_ArgumentList;
}
