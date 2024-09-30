//
// Asp executable definitions.
//

#ifndef EXECUTABLE_HPP
#define EXECUTABLE_HPP

#include "symbol.hpp"
#include "grammar.hpp"
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
    protected:

        struct InstructionInfo
        {
            Instruction *instruction;
            SourceLocation sourceLocation;
        };

    public:

        // Constructor, destructor.
        explicit Executable(SymbolTable &);
        ~Executable();

        // Check value method.
        void SetCheckValue(std::uint32_t);

        // Symbol methods.
        std::int32_t Symbol(const std::string &name) const;
        std::int32_t TemporarySymbol() const;

        // Location type definition.
        using Location = std::list<InstructionInfo>::iterator;

        // Instruction insertion methods.
        Location Insert(Instruction *, const SourceLocation &);
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
        void WriteSourceInfo(std::ostream &) const;

    protected:

        // Copy prevention.
        Executable(const Executable &) = delete;
        Executable &operator =(const Executable &) = delete;

    private:

        // Data.
        std::uint32_t checkValue = 0;
        SymbolTable &symbolTable;
        std::list<InstructionInfo> instructions;
        Location currentLocation = instructions.end();
        std::stack<Location> locationStack;
        std::map<unsigned, std::pair<Location, unsigned> > moduleLocations;
};

#endif
