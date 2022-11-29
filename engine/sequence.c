/*
 * Asp engine sequence implementation.
 */

#include "sequence.h"
#include "data.h"

static AspSequenceResult AspSequenceInsert
    (AspEngine *, AspDataEntry *sequence,
     AspDataEntry *element, AspDataEntry *value);
static bool IsSequenceType(DataType);
static bool IsElementType(DataType);

AspSequenceResult AspSequenceAppend
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *value)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine, sequence != 0 && IsSequenceType(AspDataGetType(sequence)));
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
    if (AspDataGetSequenceHeadIndex(sequence) == 0)
    {
        AspDataSetSequenceHeadIndex(sequence, elementIndex);
        AspDataSetSequenceTailIndex(sequence, elementIndex);
    }
    else
    {
        uint32_t tailIndex = AspDataGetSequenceTailIndex(sequence);
        AspDataEntry *tail = AspEntry(engine, tailIndex);
        AspDataSetElementNextIndex(tail, elementIndex);
        AspDataSetElementPreviousIndex(result.element, tailIndex);
        AspDataSetSequenceTailIndex
            (sequence, AspIndex(engine, result.element));
    }

    /* Update the sequence count. */
    uint32_t sizeChange = AspDataGetType(sequence) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;
    AspDataSetSequenceCount
        (sequence, AspDataGetSequenceCount(sequence) + sizeChange);

    return result;
}

AspSequenceResult AspSequenceInsertByIndex
    (AspEngine *engine, AspDataEntry *sequence,
     int index, AspDataEntry *value)
{
    if (index == -1 || index == (int)AspDataGetSequenceCount(sequence))
        return AspSequenceAppend(engine, sequence, value);

    /* Locate insertion point, adjusting by one for a negative index. */
    if (index < 0)
        index++;
    AspSequenceResult result = AspSequenceIndex(engine, sequence, index);
    if (result.result != AspRunResult_OK)
        return result;
    if (result.element == 0)
    {
        result.result = AspRunResult_ValueOutOfRange;
        return result;
    }

    return AspSequenceInsert(engine, sequence, result.element, value);
}

static AspSequenceResult AspSequenceInsert
    (AspEngine *engine, AspDataEntry *sequence,
     AspDataEntry *element, AspDataEntry *value)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine, sequence != 0 && IsSequenceType(AspDataGetType(sequence)));
    AspAssert
        (engine, element != 0 && IsElementType(AspDataGetType(element)));
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
        AspDataSetSequenceHeadIndex(sequence, newElementIndex);
    else
    {
        AspDataEntry *previousElement = AspEntry
            (engine, previousElementIndex);
        AspDataSetElementNextIndex(previousElement, newElementIndex);
    }
    AspDataSetElementPreviousIndex(element, newElementIndex);

    /* Update the sequence count. */
    uint32_t sizeChange = AspDataGetType(sequence) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;
    AspDataSetSequenceCount
        (sequence, AspDataGetSequenceCount(sequence) + sizeChange);

    return result;
}

bool AspSequenceErase
    (AspEngine *engine, AspDataEntry *sequence, int index,
     bool eraseValue)
{
    AspSequenceResult result = AspSequenceIndex(engine, sequence, index);
    if (result.element == 0)
        return false;

    return AspSequenceEraseElement
        (engine, sequence, result.element, eraseValue);
}

bool AspSequenceEraseElement
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *element,
     bool eraseValue)
{
    /* Update links in adjacent elements. */
    uint32_t prevIndex = AspDataGetElementPreviousIndex(element);
    uint32_t nextIndex = AspDataGetElementNextIndex(element);
    if (prevIndex == 0)
        AspDataSetSequenceHeadIndex(sequence, nextIndex);
    else
        AspDataSetElementNextIndex(AspEntry(engine, prevIndex), nextIndex);
    if (nextIndex == 0)
        AspDataSetSequenceTailIndex(sequence, prevIndex);
    else
        AspDataSetElementPreviousIndex(AspEntry(engine, nextIndex), prevIndex);

    /* Prepare to drop the element. */
    AspDataEntry *value = AspValueEntry
        (engine, AspDataGetElementValueIndex(element));
    uint32_t sizeChange = AspDataGetType(sequence) == DataType_String ?
        AspDataGetStringFragmentSize(value) : 1U;

    /* Unreference entries. */
    if (eraseValue)
        AspUnref(engine, value);
    AspUnref(engine, element);

    /* Update the sequence count. */
    AspDataSetSequenceCount
        (sequence, AspDataGetSequenceCount(sequence) - sizeChange);

    return true;
}

AspSequenceResult AspSequenceIndex
    (AspEngine *engine, AspDataEntry *sequence, int index)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    uint8_t type = AspDataGetType(sequence);
    result.result = AspAssert
        (engine, IsSequenceType(type) && type != DataType_String);

    /* Treat negative indices as counting backwards from the end. */
    int count = (int)AspDataGetSequenceCount(sequence);
    if (index < 0)
    {
        index = count + index;
        if (index < 0)
        {
            result.result = AspRunResult_ValueOutOfRange;
            return result;
        }
    }

    /* Check if the request is for the last element, as it's easier to
       fetch directly rather than traversing the whole sequence. */
    if (count > 0 && index == count - 1)
    {
        /* Return tail element. */
        result.element = AspEntry
            (engine, AspDataGetSequenceTailIndex(sequence));
        result.value = AspValueEntry
            (engine, AspDataGetElementValueIndex(result.element));
    }
    else
    {
        /* Traverse the sequence to arrive at the requested element. */
        result = AspSequenceNext(engine, sequence, 0);
        int i = 0;
        for (i = 0; i < index && result.element != 0; i++)
            result = AspSequenceNext(engine, sequence, result.element);
    }

    return result;
}

AspSequenceResult AspSequenceNext
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *element)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine, sequence != 0 && IsSequenceType(AspDataGetType(sequence)));
    result.result = AspAssert
        (engine, element == 0 || IsElementType(AspDataGetType(element)));
    if (result.result != AspRunResult_OK)
        return result;

    result.element = AspEntry
        (engine,
         element == 0 ?
         AspDataGetSequenceHeadIndex(sequence) :
         AspDataGetElementNextIndex(element));

    result.value = result.element == 0 ? 0 :
        AspValueEntry(engine, AspDataGetElementValueIndex(result.element));

    return result;
}

AspSequenceResult AspSequencePrevious
    (AspEngine *engine, AspDataEntry *sequence, AspDataEntry *element)
{
    AspSequenceResult result = {AspRunResult_OK, 0, 0};

    AspAssert
        (engine, sequence != 0 && IsSequenceType(AspDataGetType(sequence)));
    result.result = AspAssert
        (engine, element == 0 || IsElementType(AspDataGetType(element)));
    if (result.result != AspRunResult_OK)
        return result;

    result.element = AspEntry
        (engine,
         element == 0 ?
         AspDataGetSequenceTailIndex(sequence) :
         AspDataGetElementPreviousIndex(element));

    result.value = result.element == 0 ? 0 :
        AspValueEntry(engine, AspDataGetElementValueIndex(result.element));

    return result;
}

AspRunResult AspStringAppendBuffer
    (AspEngine *engine, AspDataEntry *str,
     const char *buffer, size_t bufferSize)
{
    AspRunResult result = AspRunResult_OK;

    result = AspAssert
        (engine, str != 0 && AspDataGetType(str) == DataType_String);
    if (result != AspRunResult_OK)
        return result;

    AspDataEntry *fragment = 0;
    uint32_t fragmentIndex = AspDataGetStringFragmentMaxSize();
    if (AspDataGetSequenceCount(str) != 0)
    {
        AspDataEntry *tailElement = AspEntry
            (engine, AspDataGetSequenceTailIndex(str));
        if (tailElement == 0)
            return AspRunResult_InternalError;
        fragment = AspEntry
            (engine, AspDataGetElementValueIndex(tailElement));
        if (fragment == 0)
            return AspRunResult_InternalError;
        fragmentIndex = AspDataGetStringFragmentSize(fragment);
    }

    uint32_t copySize = 0;
    for (uint32_t i = 0; i < (uint32_t)bufferSize; i += copySize)
    {
        /* Allocate a new fragment if required. */
        if (fragmentIndex >= AspDataGetStringFragmentMaxSize())
        {
            fragment = AspAllocEntry(engine, DataType_StringFragment);
            if (fragment == 0)
                return AspRunResult_OutOfDataMemory;
            fragmentIndex = 0;
        }

        /* Determine the number of bytes to copy to the fragment. */
        uint32_t remainingSize =
            AspDataGetStringFragmentMaxSize() - fragmentIndex;
        copySize = (uint32_t)bufferSize - i;
        if (copySize > remainingSize)
            copySize = remainingSize;

        /* Append the data into the fragment. */
        AspDataSetStringFragmentData
            (fragment, fragmentIndex, buffer + i, copySize);
        AspDataSetStringFragmentSize
            (fragment, fragmentIndex + copySize);

        /* Append the fragment if it was just created. Otherwise, update the
           sequence count. */
        if (fragmentIndex == 0)
        {
            AspSequenceResult appendResult = AspSequenceAppend
                (engine, str, fragment);
            if (appendResult.result != AspRunResult_OK)
                return appendResult.result;
        }
        else
        {
            AspDataSetSequenceCount
                (str, AspDataGetSequenceCount(str) + copySize);
        }

        fragmentIndex += copySize;
    }

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

static bool IsElementType(DataType type)
{
    return
        type == DataType_Element;
}
