//
// Asp symbol table definitions.
//

#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <map>
#include <string>
#include <cstdint>

class SymbolTable
{
    public:

        // Symbol fetch method.
        // If the name is empty, get a new symbol for a temporary.
        std::int32_t Symbol(const std::string & = "");

        // Symbol check method.
        bool IsDefined(const std::string &) const;

        // Symbol iteration methods.
        typedef std::map<std::string, std::int32_t> Map;
        Map::const_iterator Begin() const;
        Map::const_iterator End() const;

    private:

        // Data.
        Map symbolsByName;
        std::int32_t nextNamedSymbol = 0;
        std::int32_t nextUnnamedSymbol = -1;
};

#endif
