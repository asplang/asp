/*
 * Asp info library implementation - source info.
 */

#include "asp-info.h"
#include "symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

static bool FinishLoad(AspSourceInfo *);
static unsigned long ScanSourceFileNames
    (const AspSourceInfo *, unsigned index, bool init);
static uint32_t LoadValue(const uint8_t *p);

#define SourceInfoHeaderSize 8
static const size_t SourceInfoRecordSize = 4 * 4;
static const unsigned SourceInfo_ProgramCounterOffset = 0 * 4;
static const unsigned SourceInfo_SourceIndexOffset = 1 * 4;
static const unsigned SourceInfo_LineOffset = 2 * 4;
static const unsigned SourceInfo_ColumnOffset = 3 * 4;

struct AspSourceInfo
{
    bool owned;
    char version;
    size_t size;
    char *data;
    const uint8_t *sourceInfos;
    const char *symbolNames;
};

AspSourceInfo *AspLoadSourceInfoFromFile(const char *fileName)
{
    /* Open the source info file. */
    FILE *fp = fopen(fileName, "rb");
    if (fp == 0)
        return 0;

    /* Determine the file size. */
    int result;
    result = fseek(fp, 0, SEEK_END);
    if (result != 0)
    {
        fclose(fp);
        return 0;
    }
    long tellResult = ftell(fp);
    if (tellResult < SourceInfoHeaderSize || tellResult > SIZE_MAX)
    {
        fclose(fp);
        return 0;
    }
    rewind(fp);

    /* Read and check the header. */
    char buffer[SourceInfoHeaderSize];
    size_t readResult = fread(buffer, 1, SourceInfoHeaderSize, fp);
    if (readResult != SourceInfoHeaderSize ||
        memcmp(buffer, "AspD", 4) != 0)
    {
        fclose(fp);
        return 0;
    }

    /* Determine the format version. */
    char delimitorByte = getc(fp);
    if (feof(fp))
    {
        fclose(fp);
        return 0;
    }
    char version = 0;
    if (delimitorByte != '\0')
        ungetc(delimitorByte, fp);
    else
    {
        version = getc(fp);
        if (feof(fp))
        {
            fclose(fp);
            return 0;
        }
    }

    /* Allocate and populate the source info object with data from the file. */
    AspSourceInfo *info = (AspSourceInfo *)malloc(sizeof(AspSourceInfo));
    if (info == 0)
    {
        fclose(fp);
        return 0;
    }
    info->version = version;
    info->size = (size_t)tellResult - SourceInfoHeaderSize;
    if (version >= 0x01)
        info->size -= 2;
    info->data = (char *)malloc(info->size);
    if (info->data == 0)
    {
        free(info);
        fclose(fp);
        return 0;
    }
    info->owned = true;
    readResult = fread(info->data, 1, info->size, fp);
    fclose(fp);
    if (readResult != info->size)
    {
        AspUnloadSourceInfo(info);
        return 0;
    }

    /* Finish initialization. */
    if (!FinishLoad(info))
    {
        AspUnloadSourceInfo(info);
        return 0;
    }

    return info;
}

AspSourceInfo *AspLoadSourceInfo(const char *data, size_t size)
{
    /* Check the header. */
    if (size < SourceInfoHeaderSize ||
        memcmp(data, "AspD", 4) != 0)
        return 0;

    /* Determine the format version. */
    char version = 0;
    if (size > SourceInfoHeaderSize + 1 &&
        data[SourceInfoHeaderSize] == 0)
        version = data[SourceInfoHeaderSize + 1];

    /* Allocate and populate the source info object. Note that the
       data simply points to the caller's data. */
    AspSourceInfo *info = (AspSourceInfo *)malloc(sizeof(AspSourceInfo));
    if (info == 0)
        return 0;
    info->version = version;
    info->size = size - SourceInfoHeaderSize;
    info->data = (char *)data + SourceInfoHeaderSize;
    if (version >= 0x01)
    {
        info->size -= 2;
        info->data += 2;
    }
    info->owned = false;

    /* Finish initialization. */
    if (!FinishLoad(info))
    {
        AspUnloadSourceInfo(info);
        return 0;
    }

    return info;
}

static bool FinishLoad(AspSourceInfo *info)
{
    static const uint16_t word = 1;
    bool be = *(const char *)&word == 0;

    /* Locate the end of the list of source file names. */
    unsigned long offset = ScanSourceFileNames(info, UINT_MAX, true);
    if (offset == ULONG_MAX)
        return false;

    /* Mark the start of the source info records. */
    info->sourceInfos = (uint8_t *)info->data + offset + 1;

    /* Mark the start of the symbol names, if present. */
    info->symbolNames = 0;
    if (info->version >= 0x01)
    {
        /* Locate the end of the list of source info records. */
        for (const uint8_t *p = info->sourceInfos;
             p < (const uint8_t *)info->data + info->size;
             p += SourceInfoRecordSize)
        {
            uint32_t sourceIndex = LoadValue
                (p + SourceInfo_SourceIndexOffset);
            if (sourceIndex == UINT32_MAX)
            {
                info->symbolNames = (const char *)p + SourceInfoRecordSize;
                break;
            }
        }
    }

    return true;
}

void AspUnloadSourceInfo(AspSourceInfo *info)
{
    if (info->owned)
        free(info->data);
    free(info);
}

static unsigned long ScanSourceFileNames
    (const AspSourceInfo *info, unsigned index, bool init)
{
    /* Scan through the appropriate number of source file names. */
    unsigned count = 0;
    unsigned long offset = 0;
    while (count < index)
    {
        unsigned long offset2 = offset;
        while (info->data[offset2] != '\0')
        {
            offset2++;
            if (offset2 >= info->size)
                return ULONG_MAX;
        }
        if (offset2 == offset)
        {
            if (!init)
                return ULONG_MAX;
            break;
        }
        offset = offset2 + 1;
        count++;
    }

    /* For initialization, return the offset of the ending null file name.
       Otherwise, indicate that the index is out of range. */
    return init || info->data[offset] != '\0' ? offset : ULONG_MAX;
}

AspSourceLocation AspGetSourceLocation
    (const AspSourceInfo *info, size_t pc)
{
    /* Locate the applicable source info record. */
    const uint8_t *p = info->sourceInfos, *pp = p;
    size_t infoProgramCounter = 0;
    bool found = false;
    for (; p < (const uint8_t *)info->data + info->size;
         p += SourceInfoRecordSize)
    {
        infoProgramCounter = (size_t)LoadValue
            (p + SourceInfo_ProgramCounterOffset);
        if (infoProgramCounter >= pc)
        {
            found = true;
            break;
        }
        pp = p;
    }
    if (!found || infoProgramCounter > pc)
        p = pp;

    /* Look up the source file name. */
    AspSourceLocation result;
    uint32_t sourceIndex = LoadValue(p + SourceInfo_SourceIndexOffset);
    result.fileName = AspGetSourceFileName(info, (unsigned)sourceIndex);

    /* Fill in the source file location information. */
    result.line = LoadValue(p + SourceInfo_LineOffset);
    result.column = LoadValue(p + SourceInfo_ColumnOffset);
    return result;
}

const char *AspGetSourceFileName
    (const AspSourceInfo *info, unsigned index)
{
    unsigned long offset = ScanSourceFileNames(info, index, false);
    return offset == ULONG_MAX ? 0 : info->data + offset;
}

const char *AspGetSymbolName
    (const AspSourceInfo *info, int32_t symbol)
{
    if (info->symbolNames == 0)
        return 0;
    if (symbol < 0)
        return "";
    if (symbol == AspSystemModuleSymbol)
        return AspSystemModuleName;
    if (symbol == AspSystemArgumentsSymbol)
        return AspSystemArgumentsName;
    symbol -= AspScriptSymbolBase;
    for (const char *p = info->symbolNames;
         p < info->data + info->size && *p != '\0';
         p += strlen(p) + 1, symbol--)
    {
        if (symbol == 0)
            return p;
    }
    return "";
}

static uint32_t LoadValue(const uint8_t *buffer)
{
    uint32_t value = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        value <<= 8;
        value |= buffer[i];
    }
    return value;
}
