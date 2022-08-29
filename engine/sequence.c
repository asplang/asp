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

AspSequenceResult AspSequenceInsertByIndex
    (AspEngine *engine, AspDataEntry *list,
     int index, AspDataEntry *value)
{
    if (index == -1 || index == (int)AspDataGetSequenceCount(list))
        return AspSequenceAppend(engine, list, value);

    /* Locate insertion point, adjusting by one for a negative index. */
    if (index < 0)
        index++;
    AspSequenceResult result = AspSequenceIndex(engine, list, index);
    if (result.result != AspRunResult_OK)
        return result;
    if (result.element == 0)
    {
        result.result = AspRunResult_IndexOutOfRange;
        return result;
    }

    return AspSequenceInsert(engine, list, result.element, value);
}

AspSequenceResult AspSequenceInsert
    (AspEngine *engine, AspDataEntry *list,
     AspDataEntry *element, AspDataEntry *value)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert(engine, list != 0 && IsSequenceType(AspDataGetType(list)));
    AspAssert
        (engine, element != 0 && AspDataGetType(element) == DataType_Element);
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
    uint32_t nextElementIndex = AspIndex(engine, element);
    uint32_t previousElementIndex = AspDataGetElementPreviousIndex(element);
    uint32_t newElementIndex = AspIndex(engine, result.element);
    AspDataSetElementNextIndex(result.element, nextElementIndex);
    AspDataSetElementPreviousIndex(result.element, previousElementIndex);
    if (previousElementIndex == 0)
        AspDataSetSequenceHeadIndex(list, newElementIndex);
    else
    {
        AspDataEntry *previousElement = AspEntry
            (engine, previousElementIndex);
        AspDataSetElementNextIndex(previousElement, newElementIndex);
    }
    AspDataSetElementPreviousIndex(element, newElementIndex);

    /* Update the sequence count. */
    uint32_t sizeChange = AspDataGetType(list) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;
    AspDataSetSequenceCount(list, AspDataGetSequenceCount(list) + sizeChange);

    return result;
}

bool AspSequenceErase
    (AspEngine *engine, AspDataEntry *list, int index,
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
    (AspEngine *engine, AspDataEntry *list, int index)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    /* Treat negative indices as counting backwards from the end. */
    if (index < 0)
    {
        index = (int)AspDataGetSequenceCount(list) + index;
        if (index < 0)
        {
            result.result = AspRunResult_IndexOutOfRange;
            return result;
        }
    }

    result = AspSequenceNext(engine, list, 0);
    int i = 0;
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
        AspValueEntry(engine, AspDataGetElementValueIndex(result.element));

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
