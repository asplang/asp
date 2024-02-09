//
// Asp executable implementation.
//

#include "executable.hpp"
#include "instruction.hpp"
#include "symbols.h"
#include <iomanip>
#include <map>

using namespace std;

static const char SourceInfoVersion[1] = {'\x01'};

static void WriteItem(ostream &, const string &);
static void WriteItem(ostream &, uint32_t);

static uint32_t MaxCodeSize = 0x10000000;

Executable::Executable(SymbolTable &symbolTable) :
    symbolTable(symbolTable)
{
}

Executable::~Executable()
{
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
        delete iter->instruction;
}

void Executable::SetCheckValue(uint32_t checkValue)
{
    this->checkValue = checkValue;
}

int32_t Executable::Symbol(const string &name) const
{
    return symbolTable.Symbol(name);
}

int32_t Executable::TemporarySymbol() const
{
    return symbolTable.Symbol();
}

Executable::Location Executable::Insert
    (Instruction *instruction, const SourceLocation &sourceLocation)
{
    return instructions.insert
        (currentLocation,
         InstructionInfo{instruction, sourceLocation});
}

void Executable::PushLocation(const Location &location)
{
    locationStack.push(currentLocation);
    currentLocation = location;
}

void Executable::PopLocation()
{
    currentLocation = locationStack.top();
    locationStack.pop();
}

Executable::Location Executable::CurrentLocation() const
{
    return currentLocation;
}

void Executable::MarkModuleLocation
    (const string &name, const Location &location)
{
    auto symbol = Symbol(name);
    moduleLocations.insert(make_pair(symbol, make_pair(location, 0U)));
}

unsigned Executable::ModuleOffset(const string &name) const
{
    auto symbol = Symbol(name);
    return moduleLocations.find(symbol)->second.second;
}

void Executable::Finalize()
{
    // Assign offsets to each instruction.
    unsigned offset = 0;
    for (auto instructionIter = instructions.begin();
         instructionIter != instructions.end(); instructionIter++)
    {
        auto instruction = instructionIter->instruction;

        instruction->Offset(offset);

        // Update module location if applicable.
        for (auto moduleIter = moduleLocations.begin();
             moduleIter != moduleLocations.end(); moduleIter++)
        {
            if (moduleIter->second.first == instructionIter)
                moduleIter->second.second = offset;
        }

        offset += instruction->Size();
    }

    // Check final code size.
    if (offset > MaxCodeSize)
        throw string("Code too large");

    // Fix up target locations.
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = iter->instruction;

        if (!instruction->Fixed())
            instruction->Fix
                (instruction->TargetLocation()->instruction->Offset());
    }
}

void Executable::Write(ostream &os) const
{
    // Write header signature.
    os.write("AspE", 4);

    // Write header version information.
    os.put(ASP_COMPILER_VERSION_MAJOR);
    os.put(ASP_COMPILER_VERSION_MINOR);
    os.put(ASP_COMPILER_VERSION_PATCH);
    os.put(ASP_COMPILER_VERSION_TWEAK);

    // Write header check value.
    WriteItem(os, checkValue);

    // Write all the instructions.
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = iter->instruction;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string("Attempt to write unfixed instruction");

        instruction->Write(os);
    }
}

void Executable::WriteListing(ostream &os) const
{
    os << "Instruction listing:\n";
    SourceLocation previousSourceLocation;
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = iter->instruction;
        auto sourceLocation = iter->sourceLocation;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string("Attempt to list unfixed instruction");

        // Skip null instructions.
        if (instruction->Size() == 0)
            continue;

        // Print the source line if it has changed.
        if (sourceLocation.line != previousSourceLocation.line ||
            sourceLocation.fileName != previousSourceLocation.fileName)
        {
            if (sourceLocation.line == 0 && iter != instructions.begin())
                os << "(No source)";
            else
                os
                    << "From " << sourceLocation.fileName << ':'
                    << sourceLocation.line;
            os << ":\n";

            previousSourceLocation = sourceLocation;
        }

        // Print a listing line for the instruction.
        instruction->Print(os);
        os << '\n';
    }

    os << "\nSymbols by name:\n";
    for (auto iter = symbolTable.Begin(); iter != symbolTable.End(); iter++)
    {
        auto symbol = iter->first;
        auto value = iter->second;

        os << setw(5) << value << ' ' << symbol << '\n';
    }

    os << "\nEnd" << endl;
}

void Executable::WriteSourceInfo(ostream &os) const
{
    // Write header signature.
    os.write("AspD", 4);

    // Write header version information.
    os.put(ASP_COMPILER_VERSION_MAJOR);
    os.put(ASP_COMPILER_VERSION_MINOR);
    os.put(ASP_COMPILER_VERSION_PATCH);
    os.put(ASP_COMPILER_VERSION_TWEAK);

    // Write a null byte to distinguish the original format from newer
    // versioned formats.
    os.put('\0');

    // Write the file format version.
    os.write(SourceInfoVersion, sizeof SourceInfoVersion);

    // Write and keep track of all source file names.
    map<string, size_t> sourceFileNameIndices;
    for (auto instructionIter = instructions.begin();
         instructionIter != instructions.end(); instructionIter++)
    {
        auto instruction = instructionIter->instruction;
        auto sourceLocation = instructionIter->sourceLocation;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string
                ("Attempt to write source info for unfixed instruction");

        const auto &fileName = sourceLocation.fileName;
        if (fileName.empty())
            continue;
        auto sourceFileNameIndexIter = sourceFileNameIndices.find(fileName);
        if (sourceFileNameIndexIter == sourceFileNameIndices.end())
        {
            WriteItem(os, fileName);
            auto sourceFileNameIndex = sourceFileNameIndices.size();
            sourceFileNameIndices[fileName] = sourceFileNameIndex;
        }
    }

    // Terminate the source file names list.
    os.put('\0');

    // Write code addresses and their associated source locations.
    SourceLocation previousSourceLocation;
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = iter->instruction;
        auto sourceLocation = iter->sourceLocation;

        // Skip null instructions.
        if (instruction->Size() == 0)
            continue;

        // Write source location only when it changes.
        if (iter == instructions.begin() ||
            sourceLocation.line != previousSourceLocation.line ||
            sourceLocation.column != previousSourceLocation.column ||
            sourceLocation.fileName != previousSourceLocation.fileName)
        {
            // Determine the source file name index.
            uint32_t sourceFileNameIndex = 0;
            auto sourceFileNameIndexIter = sourceFileNameIndices.find
                (sourceLocation.fileName);
            if (sourceFileNameIndexIter != sourceFileNameIndices.end())
                sourceFileNameIndex = static_cast<uint32_t>
                    (sourceFileNameIndexIter->second);

            // Write the source location info record.
            WriteItem(os, instruction->Offset());
            WriteItem(os, sourceFileNameIndex);
            WriteItem(os, static_cast<uint32_t>(sourceLocation.line));
            WriteItem(os, static_cast<uint32_t>(sourceLocation.column));

            previousSourceLocation = sourceLocation;
        }
    }

    // Write a final source location info record for out of bounds offsets.
    uint32_t finalOffset = 0;
    if (!instructions.empty())
    {
        auto finalInstruction = instructions.back().instruction;
        finalOffset = finalInstruction->Offset() + finalInstruction->Size();
    }
    WriteItem(os, finalOffset);
    WriteItem(os, UINT32_MAX);
    WriteItem(os, 0);
    WriteItem(os, 0);

    // Write symbol names in order of their numeric value.
    map<int32_t, string> sortedSymbols;
    for (auto iter = symbolTable.Begin();
         iter != symbolTable.End(); iter++)
        sortedSymbols.emplace(iter->second, iter->first);
    unsigned symbol = 0;
    for (auto iter = sortedSymbols.begin();
         iter != sortedSymbols.end(); iter++, symbol++)
    {
        // Skip reserved symbols, as their names are already known.
        if (symbol < AspScriptSymbolBase)
            continue;

        const auto &name = iter->second;
        WriteItem(os, name);
    }

    // Terminate the symbol names list.
    os.put('\0');
}

static void WriteItem(ostream &os, const string &s)
{
    os.write(s.data(), s.size());
    os.put('\0');
}

static void WriteItem(ostream &os, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++)
    {
        uint8_t c = static_cast<uint8_t>((value >> ((3 - i) << 3)) & 0xFF);
        os.put(*reinterpret_cast<const char *>(&c));
    }
}
