//
// Asp symbol table implementation.
//

#include "symbol.hpp"
#include "reserved.h"
#include "word.h"
#include <sstream>
#include <utility>

using namespace std;

SymbolTable::SymbolTable(bool reserveSystemSymbols)
{
    // Reserve symbols used by the system if applicable.
    if (reserveSystemSymbols)
    {
        for (const AspReservedNameEntry *entry = AspNextReservedNameEntry(0);
             entry != 0; entry = AspNextReservedNameEntry(entry))
        {
            auto symbol = entry->symbol;
            auto name = entry->name;

            if (symbol >= AspReservedSymbol_End)
            {
                ostringstream oss;
                oss
                    << "Internal error: Symbol for reserved name " << name
                    << " is out of range";
                throw string(oss.str());
            }

            symbolsByName.insert(make_pair(name, symbol));
        }

        // Set the next symbol to use for a name.
        nextNamedSymbol = AspReservedSymbol_End;
    }
}

int32_t SymbolTable::Symbol(const string &name)
{
    // Return a unique symbol for the given name.
    bool empty = symbolsByName.empty();
    auto result = symbolsByName.insert(make_pair(name, nextNamedSymbol));
    if (result.second)
    {
        if (nextNamedSymbol == 0 && !empty)
            throw string("Maximum number of name symbols exceeded");
        if (nextNamedSymbol == AspSignedWordMax)
            nextNamedSymbol = 0;
        else
            nextNamedSymbol++;
    }
    return result.first->second;
}

int32_t SymbolTable::Symbol(const string &name) const
{
    auto iter = symbolsByName.find(name);
    if (iter == symbolsByName.end())
    {
        ostringstream oss;
        oss << "Symbol name '" << name << "' not found";
        throw oss.str();
    }
    return iter->second;
}

int32_t SymbolTable::TemporarySymbol()
{
    // For a temporary, return a new negative symbol.
    if (nextUnnamedSymbol == 0)
        throw string("Maximum number of temporary symbols exceeded");
    auto result = nextUnnamedSymbol;
    if (nextUnnamedSymbol == AspSignedWordMin)
        nextUnnamedSymbol = 0;
    else
        nextUnnamedSymbol--;
    return result;
}

bool SymbolTable::IsDefined(const string &name) const
{
    return symbolsByName.find(name) != symbolsByName.end();
}

SymbolTable::Map::const_iterator SymbolTable::Begin() const
{
    return symbolsByName.begin();
}

SymbolTable::Map::const_iterator SymbolTable::End() const
{
    return symbolsByName.end();
}
