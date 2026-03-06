//
// Asp symbol table implementation.
//

#include "symbol.hpp"
#include "symbols.h"
#include "word.h"
#include <sstream>
#include <utility>

using namespace std;

SymbolTable::SymbolTable(bool reserveSystemSymbols)
{
    // Reserve symbols used by the system if applicable.
    if (reserveSystemSymbols)
    {
        Symbol(AspSystemModuleName);
        Symbol(AspSystemArgumentsName);
        Symbol(AspSystemMainModuleName);

        // Set the next symbol to use for a name.
        if (nextNamedSymbol > AspScriptSymbolBase)
            throw string("Internal error: Too many reserved symbols");
        nextNamedSymbol = AspScriptSymbolBase;
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
