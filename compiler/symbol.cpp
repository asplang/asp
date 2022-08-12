//
// Asp symbol table implementation.
//

#include "symbol.hpp"
#include <utility>

using namespace std;

int32_t SymbolTable::Symbol(const string &name)
{
    // For a temporary, return a new negative symbol.
    if (name.empty())
        return nextUnnamedSymbol--;

    // Return a unique symbol for the given name.
    auto result = symbolsByName.insert(make_pair(name, nextNamedSymbol));
    if (result.second)
        nextNamedSymbol++;
    return result.first->second;
}

SymbolTable::Map::const_iterator SymbolTable::Begin() const
{
    return symbolsByName.begin();
}

SymbolTable::Map::const_iterator SymbolTable::End() const
{
    return symbolsByName.end();
}
