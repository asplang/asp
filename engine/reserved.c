/*
 * Asp reserved names implementation.
 */

#include "reserved.h"
#include <stddef.h>

static AspReservedNameEntry ReservedNameEntries[] =
{
    {AspReservedSymbol_SystemModule, "sys"},
    {AspReservedSymbol_SystemArguments, "args"},
    {AspReservedSymbol_MainModule, "__main__"},
    #ifdef ASP_FEATURE_CLASS
    {AspReservedSymbol_ClassInitialize, "__init__", AspFeatureBit_Class},
    {AspReservedSymbol_GetMethod, "__get__", AspFeatureBit_Class},
    {AspReservedSymbol_CallMethod, "__call__", AspFeatureBit_Class},
    #endif

    /* Final entry. */
    {AspReservedSymbol_End, ""}
};
static const size_t ReservedNamesEntryCount =
    sizeof ReservedNameEntries / sizeof *ReservedNameEntries;

const char *AspReservedName(int32_t symbol)
{
    for (const AspReservedNameEntry *entry = AspNextReservedNameEntry(0);
         entry != 0; entry = AspNextReservedNameEntry(entry))
    {
        if (symbol == entry->symbol)
            return entry->name;
    }

    return 0;
}

const AspReservedNameEntry *AspNextReservedNameEntry
    (const AspReservedNameEntry *entry)
{
    if (entry == 0)
        entry = ReservedNameEntries;
    else if (entry >= ReservedNameEntries &&
             entry < ReservedNameEntries + ReservedNamesEntryCount)
        entry++;
    else
        return 0;

    return *entry->name == '\0' ? 0 : entry;
}
