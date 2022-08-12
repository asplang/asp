//
// Asp executable definitions.
//

#ifndef EXECUTABLE_HPP
#define EXECUTABLE_HPP

#include "symbol.hpp"
#include <iostream>
#include <map>
#include <stack>
#include <list>
#include <string>
#include <cstdint>
#include <utility>

class SymbolTable;
class Instruction;

class Executable
{
    public:

        // Constructor, destructor.
        explicit Executable(SymbolTable &);
        ~Executable();

        // Current module methods.
        void CurrentModule(const std::string &);
        std::pair<std::string, std::int32_t> CurrentModule() const;

        // Symbol methods.
        std::int32_t Symbol(const std::string &name) const;
        std::int32_t TemporarySymbol() const;

        // Location type definition.
        typedef std::list<Instruction *>::iterator Location;

        // Instruction insertion methods.
        Location Insert(Instruction *);
        void PushLocation(const Location &);
        void PopLocation();
        Location CurrentLocation() const;

        // Module location methods.
        void MarkModuleLocation(const std::string &name, const Location &);
        unsigned ModuleOffset(const std::string &name) const;

        // Finalize method.
        void Finalize();

        // Output methods.
        void Write(std::ostream &) const;
        void WriteListing(std::ostream &) const;
        void WriteDebugInfo(std::ostream &) const;

    protected:

        // Copy prevention.
        Executable(const Executable &) = delete;
        Executable &operator =(const Executable &) = delete;

    private:

        // Data.
        SymbolTable &symbolTable;
        std::string currentModuleName;
        std::list<Instruction *> instructions;
        Location currentLocation = instructions.end();
        std::stack<Location> locationStack;
        std::map<unsigned, std::pair<Location, unsigned> > moduleLocations;
};

#endif
