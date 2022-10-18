//
// Asp executable implementation.
//

#include "executable.hpp"
#include "instruction.hpp"
#include <iomanip>

using namespace std;

static uint32_t MaxCodeSize = 0x10000000;

Executable::Executable(SymbolTable &symbolTable) :
    symbolTable(symbolTable)
{
}

Executable::~Executable()
{
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
        delete *iter;
}

void Executable::SetCheckValue(uint32_t checkValue)
{
    this->checkValue = checkValue;
}

void Executable::CurrentModule(const string &moduleName)
{
    currentModuleName = moduleName;
}

pair<string, int32_t> Executable::CurrentModule() const
{
    return make_pair
        (currentModuleName,
         symbolTable.Symbol(currentModuleName));
}

int32_t Executable::Symbol(const string &name) const
{
    return symbolTable.Symbol(name);
}

int32_t Executable::TemporarySymbol() const
{
    return symbolTable.Symbol();
}

Executable::Location Executable::Insert(Instruction *instruction)
{
    return instructions.insert(currentLocation, instruction);
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
        auto instruction = *instructionIter;
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
    for (auto instructionIter = instructions.begin();
         instructionIter != instructions.end(); instructionIter++)
    {
        auto instruction = *instructionIter;
        if (!instruction->Fixed())
            instruction->Fix((*instruction->TargetLocation())->Offset());
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
    {
        unsigned i = 4;
        while (i--)
            os << static_cast<char>((checkValue >> (i << 3)) & 0xFF);
    }

    // Write all the instructions.
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = *iter;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string("Attempt to write unfixed instruction");

        instruction->Write(os);
    }
}

void Executable::WriteListing(ostream &os) const
{
    os << "Instruction listing:\n";
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = *iter;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string("Attempt to list unfixed instruction");

        // Skip null instructions.
        if (instruction->Size() == 0)
            continue;

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

void Executable::WriteDebugInfo(ostream &os) const
{
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++)
    {
        auto instruction = *iter;

        // Ensure any required fixing has been applied to the instruction.
        if (!instruction->Fixed())
            throw string("Attempt to write debug info for unfixed instruction");

        // TODO: Implement.
    }
}
