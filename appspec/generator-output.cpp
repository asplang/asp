//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "symbol.hpp"
#include <iomanip>
#include <set>

using namespace std;

void Generator::WriteCompilerSpec(ostream &os)
{
    // Write each name only once, in order of assigned symbol.
    set<int> writtenSymbols;
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto symbol = iter->first;
        auto &functionDefinition = iter->second;

        auto writtenIter = writtenSymbols.find(symbol);
        if (writtenIter == writtenSymbols.end())
        {
            os << functionDefinition.name << endl;
            writtenSymbols.insert(symbol);
        }
    }
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = iter->second;

        for (auto parameterIter = functionDefinition.parameterNames.begin();
             parameterIter != functionDefinition.parameterNames.end();
             parameterIter++)
        {
            auto &parameterName = *parameterIter;
            auto parameterSymbol = symbolTable.Symbol(parameterName);

            auto writtenIter = writtenSymbols.find(parameterSymbol);
            if (writtenIter == writtenSymbols.end())
            {
                os << parameterName << endl;
                writtenSymbols.insert(parameterSymbol);
            }
        }
    }
}

void Generator::WriteApplicationHeader(ostream &os)
{
    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#ifndef ASP_APP_" << baseFileName << "_DEF_H\n"
           "#define ASP_APP_" << baseFileName << "_DEF_H\n\n"
           "#include \"asp.h\"\n\n"
           "#ifdef __cplusplus\n"
           "extern \"C\" {\n"
           "#endif\n\n"
           "extern AspAppSpec AspAppSpec_" << baseFileName << ";\n";

    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = iter->second;

        os
            << "\nAspRunResult " << functionDefinition.internalName << "\n"
               "    (AspEngine *,";
        if (!functionDefinition.parameterNames.empty())
            os << "\n";

        for (auto parameterIter = functionDefinition.parameterNames.begin();
             parameterIter != functionDefinition.parameterNames.end();
             parameterIter++)
        {
            auto &parameterName = *parameterIter;
            os << "     AspDataEntry *" << parameterName << ",\n";
        }

        if (functionDefinition.parameterNames.empty())
            os << ' ';
        else
            os << "     ";

        os << "AspDataEntry **returnValue);\n";
    }

    os
        << "\n"
           "#ifdef __cplusplus\n"
           "}\n"
           "#endif\n\n"
           "#endif\n";
}

void Generator::WriteApplicationCode(ostream &os)
{
    auto oldFlags = os.flags();
    auto oldFill = os.fill();

    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#include \"" << baseFileName << ".h\"\n"
           "#include <stdint.h>\n\n"
           "static AspRunResult AspDispatch_" << baseFileName
        << "\n    (AspEngine *engine, int32_t symbol, AspDataEntry *ns,\n"
           "     AspDataEntry **returnValue)\n"
           "{\n"
           "    switch (symbol)\n"
           "    {\n";

    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto symbol = iter->first;
        auto &functionDefinition = iter->second;

        os
            << "        case " << symbol << ":\n"
            << "        {\n";

        for (auto parameterIter = functionDefinition.parameterNames.begin();
             parameterIter != functionDefinition.parameterNames.end();
             parameterIter++)
        {
            auto &parameterName = *parameterIter;
            int32_t parameterSymbol = symbolTable.Symbol(parameterName);

            os
                << "            AspDataEntry *" << parameterName << " = "
                << "AspParameterValue(engine, ns, "
                << parameterSymbol << ");\n";
        }

        os
            << "            return " << functionDefinition.internalName
            << "(engine, ";

        for (auto parameterIter = functionDefinition.parameterNames.begin();
             parameterIter != functionDefinition.parameterNames.end();
             parameterIter++)
        {
            auto &parameterName = *parameterIter;
            os << parameterName << ", ";
        }

        os
            << "returnValue);\n"
               "        }\n";
    }

    os
        << "    }\n"
           "    return AspRunResult_UndefinedAppFunction;\n"
           "}\n\n"
           "AspAppSpec AspAppSpec_" << baseFileName << " =\n"
           "{"
        << hex << setprecision(2) << setfill('0');

    unsigned specByteCount = 0;
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto symbol = iter->first;
        auto &functionDefinition = iter->second;

        os
            << "\n    \"\\x" << setw(2)
            << functionDefinition.parameterNames.size();
        specByteCount++;

        for (auto parameterIter = functionDefinition.parameterNames.begin();
             parameterIter != functionDefinition.parameterNames.end();
             parameterIter++)
        {
            auto &parameterName = *parameterIter;
            int32_t parameterSymbol = symbolTable.Symbol(parameterName);
            uint32_t word = *reinterpret_cast<uint32_t *>(&parameterSymbol);
            for (unsigned i = 0; i < 4; i++)
            {
                uint32_t byte = (word >> ((3 - i) * 8)) & 0xFF;
                if (byte == 0)
                    os << "\\0";
                else
                    os << "\\x" << setw(2) << byte;
            }
            specByteCount += 4;
        }
        os << '"';
    }
    os << dec;

    // TODO: Calculate CRC.
    uint32_t crc = 0;

    os
        << ",\n    " << specByteCount
        << ", 0x" << hex << setfill('0') << setw(4) << crc
        << ", AspDispatch_" << baseFileName << "\n"
           "};\n";

    os.flags(oldFlags);
    os.fill(oldFill);
}

void Generator::ReportError(const string &error)
{
    errorStream
        << currentSourceLocation.line << ':'
        << currentSourceLocation.column << ": Error: "
        << error << endl;
    errorCount++;
}
