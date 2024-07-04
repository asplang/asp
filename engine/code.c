/*
 * Asp engine code implementation.
 */

#include "code.h"
#include <stdint.h>

static void UpdateAges(AspEngine *);

AspRunResult AspLoadCodeBytes
    (AspEngine *engine, uint8_t *bytes, size_t count)
{
    if (engine->cachedCodePageCount == 0)
    {
        if (engine->pc + count > engine->codeEndIndex)
            return AspRunResult_BeyondEndOfCode;

        while (count--)
            *bytes++ = *(engine->code + engine->pc++);
        return AspRunResult_OK;
    }

    /* Handle paged code. */
    uint32_t offset = engine->headerIndex + engine->pc;
    while (count--)
    {
        AspRunResult checkResult = AspValidateCodeAddress(engine, engine->pc);
        if (checkResult != AspRunResult_OK)
            return checkResult;

        uint8_t *page =
            engine->codeArea +
            engine->cachedCodePageIndex * engine->codePageSize;
        uint32_t pageOffset = offset++ % engine->codePageSize;
        engine->pc++;
        *bytes++ = *(page + pageOffset);
    }

    return AspRunResult_OK;
}

AspRunResult AspValidateCodeAddress(AspEngine *engine, uint32_t address)
{
    if (engine->cachedCodePageCount == 0)
    {
        if (address > engine->codeEndIndex)
            return AspRunResult_BeyondEndOfCode;
    }
    else
    {
        uint32_t offset = engine->headerIndex + address;
        const AspCodePageEntry *entry =
            engine->cachedCodePages + engine->cachedCodePageIndex;
        if (offset < entry->offset ||
            offset >= entry->offset + engine->codePageSize)
        {
            AspRunResult loadResult = AspLoadCodePage(engine, offset);
            if (loadResult != AspRunResult_OK)
                return loadResult;
        }

        /* Ensure the program counter is in bounds if possible (i.e., if the
           last page has been loaded). */
        if (engine->codeEndKnown && engine->pc >= engine->codeEndIndex)
            return AspRunResult_BeyondEndOfCode;
    }

    return AspRunResult_OK;
}

AspRunResult AspLoadCodePage(AspEngine *engine, uint32_t offset)
{
    /* Determine which page to load. */
    uint8_t codePageIndex = offset / engine->codePageSize;

    /* Determine whether the page is already cached. */
    for (uint8_t i = 0; i < engine->cachedCodePageCount; i++)
    {
        const AspCodePageEntry *entry = engine->cachedCodePages + i;

        if (entry->age >= 0 &&
            offset >= entry->offset &&
            offset < entry->offset + engine->codePageSize)
        {
            if (engine->cachedCodePageIndex != i)
            {
                engine->cachedCodePageIndex = i;
                UpdateAges(engine);
            }
            return AspRunResult_OK;
        }
    }

    /* Find the least recently used cache page. */
    int8_t oldestAge = 0;
    for (uint8_t i = 0; i < engine->cachedCodePageCount; i++)
    {
        AspCodePageEntry *entry = engine->cachedCodePages + i;

        /* Check for a previously unused entry. */
        if (entry->age < 0)
        {
            engine->cachedCodePageIndex = i;
            break;
        }

        if (i == 0 || entry->age > oldestAge)
        {
            oldestAge = entry->age;
            engine->cachedCodePageIndex = i;
        }
    }
    uint32_t codeOffset = codePageIndex * engine->codePageSize;
    engine->cachedCodePages[engine->cachedCodePageIndex].offset = codeOffset;
    UpdateAges(engine);

    /* Read the page from offline storage into the least recently used cache
       page. */
    size_t pageSize = engine->codePageSize;
    if (engine->codePageReadCount < SIZE_MAX)
        engine->codePageReadCount++;
    AspRunResult readResult = engine->codeReader
        (engine->pagedCodeId, codeOffset, &pageSize,
         engine->codeArea +
         engine->cachedCodePageIndex * engine->codePageSize);
    if (readResult != AspRunResult_OK)
        return readResult;
    if (codeOffset == 0 && pageSize < engine->headerIndex)
        return AspRunResult_BeyondEndOfCode;

    /* Determine the entire code size if possible. */
    if (pageSize != engine->codePageSize)
    {
        size_t endIndex = codeOffset + pageSize - engine->headerIndex;
        if (!engine->codeEndKnown || endIndex < engine->codeEndIndex)
        {
            engine->codeEndIndex = endIndex;
            engine->codeEndKnown = true;
        }
    }

    return AspRunResult_OK;
}

static void UpdateAges(AspEngine *engine)
{
    for (unsigned i = 0; i < engine->cachedCodePageCount; i++)
    {
        AspCodePageEntry *entry = engine->cachedCodePages + i;

        if (i == engine->cachedCodePageIndex)
            entry->age = 0;
        else if (entry->age >= 0)
            entry->age++;
    }
}
