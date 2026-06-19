//
// Asp reserved system symbols implementation.
//

#include "system.hpp"
#include "reserved.h"

using namespace std;

void ReserveSystemSymbols(SymbolTable &symbolTable, AspFeatureBits featureBits)
{
    // Reserve symbols used by the system if applicable.
    for (const AspReservedNameEntry *entry = AspNextReservedNameEntry(0);
         entry != 0; entry = AspNextReservedNameEntry(entry))
    {
        auto symbol = entry->symbol;
        auto name = entry->name;

        if (entry->featureBits != 0 &&
            (entry->featureBits & featureBits) == 0)
            continue;

        symbolTable.ReserveSystemSymbol(symbol, name);
    }
}
