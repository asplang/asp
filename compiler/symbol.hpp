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

        // Constructor.
        SymbolTable(std::int32_t reservedEnd = 0);

        // System symbol reservation method.
        void ReserveSystemSymbol(std::int32_t, const std::string &);

        // Symbol fetch method. If not found, the symbol is assigned a value.
        std::int32_t Symbol(const std::string &);

        // Symbol fetch method. If not found, an exception is thrown.
        std::int32_t Symbol(const std::string &) const;

        // Temporary symbol fetch method. A new value is returned each time.
        std::int32_t TemporarySymbol();

        // Symbol check method.
        bool IsDefined(const std::string &) const;

        // Symbol iteration methods.
        using Map = std::map<std::string, std::int32_t>;
        Map::const_iterator Begin() const;
        Map::const_iterator End() const;

    private:

        // Data.
        Map symbolsByName;
        std::int32_t nextNamedSymbol;
        std::int32_t nextUnnamedSymbol = -1;
};

#endif
