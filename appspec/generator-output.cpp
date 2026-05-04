//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "appspec.h"
#include "data.h"
#include "crc.h"
#include <set>
#include <sstream>
#include <iomanip>

using namespace std;

static void WriteValue(ostream &, unsigned *specByteCount, const Literal &);
static void ContributeValue
    (const crc_spec_t &, crc_session_t &, const Literal &);

template <class T>
static void Write(ostream &os, T value)
{
    unsigned i = sizeof value;
    while (i--)
        os << static_cast<char>((value >> (i << 3)) & 0xFF);
}

static void WriteStringEscapedHexByte(ostream &os, uint8_t value)
{
    auto oldFlags = os.flags();
    auto oldFill = os.fill();

    os << hex << uppercase << setprecision(2) << setfill('0');
    if (value == 0)
        os << "\\0";
    else
        os << "\\x" << setw(2) << static_cast<unsigned>(value);

    os.flags(oldFlags);
    os.fill(oldFill);
}

template <class T>
static void WriteStringEscapedHex(ostream &os, T value)
{
    unsigned i = sizeof value;
    while (i--)
    {
        auto byte = static_cast<uint8_t>((value >> (i << 3)) & 0xFF);
        WriteStringEscapedHexByte(os, byte);
    }
}

void Generator::WriteCompilerSpec(ostream &os)
{
    if (!finalized)
        throw string("Internal error: Not finalized");

    // Write the specification's header, including check value.
    os.write("AspS", 4);
    os.put(static_cast<char>(compilerAppSpecVersion));
    Write(os, CheckValue());

    // Write the feature bits if applicable.
    if (compilerAppSpecVersion >= 3u)
        os.put(static_cast<char>(featureBits));

    // If applicable, assign symbols to import names, writing each name only
    // once, all followed by a separator to separate them from the remaining
    // symbol names.
    char delim = compilerAppSpecVersion >= 2u ? ' ' : '\n';
    for (const auto &importEntry: imports)
    {
        const auto &importName = importEntry.first;

        if (symbolTable.IsDefined(importName))
            continue;

        symbolTable.Symbol(importName);
        os << importName << delim;
    }
    if (compilerAppSpecVersion >= 2u)
        os << delim;

    // Assign symbols, to variable and function names first, then to parameter
    // names, writing each name only once, in order of assigned symbol.
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &name = definitionEntry.first;

            if (symbolTable.IsDefined(name))
                continue;

            symbolTable.Symbol(name);
            os << name << delim;
        }
    }
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &definition = definitionEntry.second.get();
            const auto functionDefinition =
                dynamic_cast<const FunctionDefinition *>(definition);
            if (functionDefinition == nullptr)
                continue;

            const auto &parameters = functionDefinition->Parameters();

            for (auto parameterIter = parameters.ParametersBegin();
                 parameterIter != parameters.ParametersEnd();
                 parameterIter++)
            {
                const auto &parameter = **parameterIter;
                const auto &parameterName = parameter.Name();

                if (symbolTable.IsDefined(parameterName))
                    continue;

                symbolTable.Symbol(parameterName);
                os << parameterName << delim;
            }
        }
    }

    symbolsAssigned = true;
}

void Generator::WriteApplicationHeader(ostream &os) const
{
    if (!symbolsAssigned)
        throw string("Internal error: Symbol not assigned");

    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#ifndef ASP_APP_" << variableBaseName << "_DEF_H\n"
           "#define ASP_APP_" << variableBaseName << "_DEF_H\n\n"
           "#include <asp.h>\n\n"
           "#ifdef __cplusplus\n"
           "extern \"C\" {\n"
           "#endif\n\n"
           "extern AspAppSpec AspAppSpec_" << variableBaseName << ";\n\n";

    // Write symbol macro definitions.
    for (auto iter = symbolTable.Begin(); iter != symbolTable.End(); iter++)
    {
        const auto &name = iter->first;
        const auto &symbol = iter->second;

        os
            << "#define ASP_APP_" << variableBaseName << "_SYM_" << name
            << ' ' << symbol << '\n';
    }

    // Write application function declarations.
    set<string> functionInternalNames;
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &definition = definitionEntry.second.get();
            const auto functionDefinition =
                dynamic_cast<const FunctionDefinition *>(definition);
            if (functionDefinition == nullptr)
                continue;
            const auto &functionInternalName =
                functionDefinition->InternalName();

            // Prevent writting duplicate declarations.
            auto fiter = functionInternalNames.find(functionInternalName);
            if (fiter != functionInternalNames.end())
                continue;

            os << '\n';
            if (functionDefinition->IsLibraryInterface())
                os << "ASP_LIB_API ";
            os
                << "AspRunResult " << functionDefinition->InternalName() << "\n"
                   "    (AspEngine *,";

            const auto &parameters = functionDefinition->Parameters();

            if (!parameters.ParametersEmpty())
                os << "\n";

            for (auto parameterIter = parameters.ParametersBegin();
                 parameterIter != parameters.ParametersEnd();
                 parameterIter++)
            {
                const auto &parameter = **parameterIter;

                os << "     AspDataEntry *_" << parameter.Name() << ',';
                if (parameter.IsGroup())
                    os
                        << " /* "
                        << (parameter.IsTupleGroup() ? "tuple" : "dictionary")
                        << " group */";
                os << '\n';
            }

            if (parameters.ParametersEmpty())
                os << ' ';
            else
                os << "     ";

            os << "AspDataEntry **returnValue);\n";
        }
    }

    os
        << "\n"
           "#ifdef __cplusplus\n"
           "}\n"
           "#endif\n\n"
           "#endif\n";
}

void Generator::WriteApplicationCode(ostream &os) const
{
    if (!symbolsAssigned)
        throw string("Internal error: Symbol not assigned");

    static string engineAppSpec1EngineVersion = "1.2.3.0";
    static string engineAppSpec1EngineVersionHex = "0x01020300";
    static string engineAppSpec1EngineVersionGoodCheck =
        string("#if ASP_VERSION >= ") + engineAppSpec1EngineVersionHex;
    static string engineAppSpec1EngineVersionBadCheck =
        string("#if ASP_VERSION < ") + engineAppSpec1EngineVersionHex;

    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#include \"" << fileBaseName << ".h\"\n"
           "#include <stdint.h>\n";

    // Write a minimum engine version check if applicable.
    if (engineAppSpecVersion >= 1u)
    {
        os
            << "\n" << engineAppSpec1EngineVersionBadCheck << "\n"
               "#error Asp engine must be version "
            << engineAppSpec1EngineVersion << " or greater\n"
            << "#endif\n";
    }

    // Write the dispatch function.
    os
        << "\nstatic AspRunResult AspDispatch_" << variableBaseName
        << "\n    (AspEngine *engine,\n";
    if (engineAppSpecVersion == 0)
        os << "     " << engineAppSpec1EngineVersionGoodCheck << '\n';
    os << "     int32_t moduleSymbol,";
    if (engineAppSpecVersion == 0)
        os << "\n     #endif\n    ";
    os
        << " int32_t functionSymbol,\n"
           "     AspDataEntry *ns, AspDataEntry **returnValue)\n"
           "{\n";
    if (engineAppSpecVersion == 0)
        os << "    " << engineAppSpec1EngineVersionGoodCheck << '\n';
    os
        << "    switch (moduleSymbol)\n"
           "    {\n";
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        const auto &moduleName = moduleEntry.second.moduleName;
        auto moduleSymbol = -moduleIdTable.Symbol(moduleName);

        os << "        case " << moduleSymbol << ":\n";
        if (engineAppSpecVersion == 0)
            os << "    #endif\n";
        os
            << "            switch (functionSymbol)\n"
               "            {\n"
               "                default:\n"
               "                    break;\n";

        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &name = definitionEntry.first;
            const auto &definition = definitionEntry.second.get();
            const auto functionDefinition =
                dynamic_cast<const FunctionDefinition *>(definition);
            if (functionDefinition == nullptr)
                continue;

            auto symbol = symbolTable.Symbol(name);

            os
                << "                case " << symbol << ":\n"
                << "                {\n";

            const auto &parameters = functionDefinition->Parameters();

            for (auto parameterIter = parameters.ParametersBegin();
                 parameterIter != parameters.ParametersEnd();
                 parameterIter++)
            {
                const auto &parameter = **parameterIter;

                const auto &parameterName = parameter.Name();
                auto parameterSymbol = symbolTable.Symbol(parameterName);

                if (parameter.IsGroup())
                {
                    os
                        << "                    AspParameterResult _"
                        << parameterName
                        << " = AspGroupParameterValue(engine, ns, "
                        << parameterSymbol << ", "
                        << (parameter.IsTupleGroup() ? "false" : "true")
                        << ");\n"
                        << "                    if (_" << parameterName
                        << ".result != AspRunResult_OK)\n"
                        << "                        return _" << parameterName
                        << ".result;\n";
                }
                else
                {
                    os
                        << "                    AspDataEntry *_"
                        << parameterName
                        << " = AspParameterValue(engine, ns, "
                        << parameterSymbol << ");\n"
                        << "                    if (_" << parameterName
                        << " == 0)\n"
                        << "                        "
                        << "return AspRunResult_OutOfDataMemory;\n";
                }
            }

            os
                << "                    return "
                << functionDefinition->InternalName()
                << "(engine, ";

            for (auto parameterIter = parameters.ParametersBegin();
                 parameterIter != parameters.ParametersEnd();
                 parameterIter++)
            {
                const auto &parameter = **parameterIter;

                os << '_' << parameter.Name();
                if (parameter.IsGroup())
                    os << ".value";
                os << ", ";
            }

            os
                << "returnValue);\n"
                   "                }\n";
        }

        os << "            }\n";
        if (engineAppSpecVersion == 0)
            os << "    " << engineAppSpec1EngineVersionGoodCheck << '\n';
        os << "            break;\n";
    }
    os << "    }\n";
    if (engineAppSpecVersion == 0)
        os << "    #endif\n";
    os
        << "    return AspRunResult_UndefinedAppFunction;\n"
           "}\n";

    // Write the application specification structure.
    os
        << "\nAspAppSpec AspAppSpec_" << variableBaseName << " =\n"
           "{";
    unsigned specByteCount = 0;
    if (engineAppSpecVersion >= 1u)
    {
        os << "\n    \"\\xFF\\xFF";
        specByteCount += 2;
        WriteStringEscapedHex(os, engineAppSpecVersion);
        specByteCount += sizeof engineAppSpecVersion;
        if (engineAppSpecVersion >= 2u)
        {
            WriteStringEscapedHex(os, featureBits);
            specByteCount += sizeof featureBits;
        }
        auto moduleCount = static_cast<uint32_t>
            (definitionsByModuleKey.size()) - 1u;
        WriteStringEscapedHex(os, moduleCount);
        specByteCount += sizeof moduleCount;
        os << '"';
    }
    for (const auto &importEntry: imports)
    {
        const auto &importName = importEntry.first;
        const auto &moduleName = importEntry.second.first;

        os << "\n    \"";

        // Write the import entry prefix.
        WriteStringEscapedHex
            (os, static_cast<uint8_t>(AppSpecPrefix_Import));
        specByteCount++;

        // Write the module identifier.
        auto moduleSymbol = -moduleIdTable.Symbol(moduleName);
        WriteStringEscapedHex
            (os, *reinterpret_cast<uint32_t *>(&moduleSymbol));
        specByteCount += sizeof moduleSymbol;

        os << '"';
    }
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        const auto &moduleName = moduleEntry.second.moduleName;

        // Write a module entry if applicable.
        if (!moduleName.empty())
        {
            os << "\n    \"";

            // Write the module entry prefix.
            WriteStringEscapedHex
                (os, static_cast<uint8_t>(AppSpecPrefix_Module));
            specByteCount++;

            os << '"';
        }

        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &name = definitionEntry.first;
            const auto &definition = definitionEntry.second.get();
            const auto assignment =
                dynamic_cast<const Assignment *>(definition);
            const auto functionDefinition =
                dynamic_cast<const FunctionDefinition *>(definition);

            // Skip symbol assignments when writing engine app spec version 1
            // and greater.
            if (engineAppSpecVersion >= 1u && assignment != nullptr &&
                assignment->Value() == nullptr)
                continue;

            os << "\n    \"";

            // Write the rest of the entry.
            if (assignment != nullptr)
            {
                const auto &value = assignment->Value();

                // Write the assignment entry prefix.
                WriteStringEscapedHex
                    (os,
                     static_cast<uint8_t>
                        (value == nullptr ?
                         AppSpecPrefix_Symbol : AppSpecPrefix_Variable));
                specByteCount++;

                // Write the variable's symbol if applicable.
                if (engineAppSpecVersion >= 1u)
                {
                    auto nameSymbol = symbolTable.Symbol(name);
                    WriteStringEscapedHex
                        (os, *reinterpret_cast<uint32_t *>(&nameSymbol));
                    specByteCount += sizeof nameSymbol;
                }

                // Write the assigned value if applicable.
                if (value != nullptr)
                    WriteValue(os, &specByteCount, *value);
            }
            else if (functionDefinition != nullptr)
            {
                auto &parameters = functionDefinition->Parameters();

                // Write the function entry prefix and/or parameter count.
                auto parameterCount = parameters.ParametersSize();
                if (parameterCount > static_cast<size_t>
                    (AppSpecPrefix_MaxFunctionParameterCount))
                {
                    // Write the entry prefix separately.
                    WriteStringEscapedHex
                        (os, static_cast<uint8_t>(AppSpecPrefix_Function));
                    specByteCount++;

                    // Write a 4-byte parameter count.
                    WriteStringEscapedHex
                        (os, static_cast<uint32_t>(parameterCount));
                    specByteCount += 4;
                }
                else
                {
                    // Write a single-byte parameter count which stands in for
                    // the entry prefix.
                    WriteStringEscapedHex
                        (os, static_cast<uint8_t>(parameterCount));
                    specByteCount++;
                }

                // Write the function's symbol if applicable.
                if (engineAppSpecVersion >= 1u)
                {
                    auto nameSymbol = symbolTable.Symbol(name);
                    WriteStringEscapedHex
                        (os, *reinterpret_cast<uint32_t *>(&nameSymbol));
                    specByteCount += sizeof nameSymbol;
                }

                // Write the function's parameter specifications.
                for (auto parameterIter = parameters.ParametersBegin();
                     parameterIter != parameters.ParametersEnd();
                     parameterIter++)
                {
                    const auto &parameter = **parameterIter;

                    // Prepare to write the parameter symbol.
                    int32_t parameterSymbol = symbolTable.Symbol
                        (parameter.Name());
                    auto word = *reinterpret_cast<uint32_t *>
                        (&parameterSymbol);

                    // Add any applicable flags to the parameter symbol word.
                    uint32_t parameterType = 0;
                    const auto &defaultValue = parameter.DefaultValue();
                    if (defaultValue != nullptr)
                        parameterType = AppSpecParameterType_Defaulted;
                    else if (parameter.IsTupleGroup())
                        parameterType = AppSpecParameterType_TupleGroup;
                    else if (parameter.IsDictionaryGroup())
                        parameterType = AppSpecParameterType_DictionaryGroup;
                    word |= parameterType << AspWordBitSize;

                    // Write the parameter symbol and flags.
                    WriteStringEscapedHex(os, word);
                    specByteCount += sizeof word;

                    // Write the parameter's default value if applicable.
                    if (defaultValue != nullptr)
                        WriteValue(os, &specByteCount, *defaultValue);
                }
            }

            os << '"';
        }
    }

    {
        auto oldFlags = os.flags();
        auto oldFill = os.fill();

        os
            << ",\n    " << specByteCount
            << hex << uppercase << setprecision(4) << setfill('0')
            << ", 0x" << setw(4) << CheckValue() << dec
            << ", AspDispatch_" << variableBaseName << "\n"
               "};\n";

        os.flags(oldFlags);
        os.fill(oldFill);
    }
}

uint32_t Generator::CheckValue() const
{
    if (!finalized)
        throw string("Internal error: Check value not computed");
    return checkValue;
}

uint32_t Generator::ComputeCheckValue() const
{
    // Use CRC-32/ISO-HDLC for computing a check value.
    const auto crcSpec = crc_make_spec
        (32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF);
    crc_session_t crcSession;
    crc_start(&crcSpec, &crcSession);

    static const string
        CheckValueFeatureBitsPrefix = "f",
        CheckValueModulePrefix = ".",
        CheckValueVariablePrefix = "\v",
        CheckValueFunctionPrefix = "\f",
        CheckValueParameterPrefix = "(";

    // Contribute the feature set to the check value if applicable.
    if (featureBits != 0)
    {
        crc_add
            (&crcSpec, &crcSession,
             CheckValueFeatureBitsPrefix.c_str(),
             static_cast<unsigned>(CheckValueFeatureBitsPrefix.size()));
        crc_add(&crcSpec, &crcSession, &featureBits, 1);
    }

    // Contribute each definition to the check value.
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        const auto &moduleKey = moduleEntry.first;

        // For imported modules, contribute the import names that make up the
        // module key, each terminated by a null byte.
        if (!moduleKey.empty())
        {
            crc_add
                (&crcSpec, &crcSession,
                 CheckValueModulePrefix.c_str(),
                 static_cast<unsigned>(CheckValueModulePrefix.size()));
        }
        for (const auto &importName: moduleKey)
        {
            crc_add
                (&crcSpec, &crcSession,
                 importName.c_str(), static_cast<unsigned>(importName.size()));
            crc_add(&crcSpec, &crcSession, "", 1);
        }

        for (const auto &definitionEntry: *moduleEntry.second.definitions)
        {
            const auto &name = definitionEntry.first;
            const auto &definition = definitionEntry.second.get();
            const auto assignment =
                dynamic_cast<const Assignment *>(definition);
            const auto functionDefinition =
                dynamic_cast<const FunctionDefinition *>(definition);

            if (assignment != nullptr)
            {
                const auto &value = assignment->Value();

                // Contribute the variable/symbol name.
                crc_add
                    (&crcSpec, &crcSession,
                     CheckValueVariablePrefix.c_str(),
                     static_cast<unsigned>(CheckValueVariablePrefix.size()));
                crc_add
                    (&crcSpec, &crcSession,
                     name.c_str(), static_cast<unsigned>(name.size()));

                // Contribute the variable's value if applicable.
                if (value != nullptr)
                    ContributeValue(crcSpec, crcSession, *value);
            }
            else if (functionDefinition != nullptr)
            {
                // Contribute the function name.
                crc_add
                    (&crcSpec, &crcSession,
                     CheckValueFunctionPrefix.c_str(),
                     static_cast<unsigned>(CheckValueFunctionPrefix.size()));
                crc_add
                    (&crcSpec, &crcSession,
                     name.c_str(), static_cast<unsigned>(name.size()));

                // Contribute the function parameters.
                const auto &parameters = functionDefinition->Parameters();
                for (auto parameterIter = parameters.ParametersBegin();
                     parameterIter != parameters.ParametersEnd();
                     parameterIter++)
                {
                    const auto &parameter = **parameterIter;
                    const auto &parameterName = parameter.Name();

                    // Contribute the parameter name.
                    crc_add
                        (&crcSpec, &crcSession,
                         CheckValueParameterPrefix.c_str(),
                         static_cast<unsigned>
                            (CheckValueParameterPrefix.size()));
                    crc_add
                        (&crcSpec, &crcSession,
                         parameterName.c_str(),
                         static_cast<unsigned>(parameterName.size()));

                    // Contribute the default value if present.
                    const auto &defaultValue = parameter.DefaultValue();
                    if (defaultValue != nullptr)
                        ContributeValue(crcSpec, crcSession, *defaultValue);
                }
            }
            else
                throw string("Internal error");
        }
    }
    return static_cast<uint32_t>(crc_finish(&crcSpec, &crcSession));
}

static void WriteValue
    (ostream &os, unsigned *specByteCount, const Literal &literal)
{
    auto valueType = literal.GetType();
    WriteStringEscapedHex(os, static_cast<uint8_t>(valueType));
    (*specByteCount)++;

    switch (valueType)
    {
        case AppSpecValueType_Boolean:
        {
            auto value = literal.BooleanValue();
            WriteStringEscapedHex
                (os, static_cast<uint8_t>(value));
            (*specByteCount)++;
            break;
        }

        case AppSpecValueType_Integer:
        {
            int32_t value = literal.IntegerValue();
            WriteStringEscapedHex
                (os, *reinterpret_cast<uint32_t *>(&value));
            *specByteCount += sizeof value;
            break;
        }

        case AppSpecValueType_Float:
        {
            static const uint16_t word = 1;
            bool be = *(const char *)&word == 0;

            auto value = literal.FloatValue();
            auto data = reinterpret_cast<const uint8_t *>(&value);
            for (unsigned i = 0; i < sizeof value; i++)
            {
                auto b = data[be ? i : sizeof value - 1 - i];
                WriteStringEscapedHex(os, b);
            }
            *specByteCount += sizeof value;
            break;
        }

        case AppSpecValueType_String:
        {
            auto value = literal.StringValue();
            auto valueSize = static_cast<uint32_t>
                (value.size());
            WriteStringEscapedHex(os, valueSize);
            for (unsigned i = 0; i < valueSize; i++)
                WriteStringEscapedHex
                    (os, static_cast<uint8_t>(value[i]));
            *specByteCount += sizeof valueSize + valueSize;
            break;
        }
    }
}

static void ContributeValue
    (const crc_spec_t &crcSpec, crc_session_t &crcSession,
     const Literal &literal)
{
    auto valueType = literal.GetType();
    auto b = static_cast<uint8_t>(valueType);
    crc_add(&crcSpec, &crcSession, &b, 1);

    switch (valueType)
    {
        case AppSpecValueType_Boolean:
        {
            uint8_t b = literal.BooleanValue() ? 1 : 0;
            crc_add(&crcSpec, &crcSession, &b, 1);
            break;
        }

        case AppSpecValueType_Integer:
        {
            int32_t value = literal.IntegerValue();
            auto uValue = *reinterpret_cast<uint32_t *>(&value);
            for (unsigned i = 0; i < sizeof value; i++)
            {
                uint8_t b =
                    uValue >> 8 * (sizeof value - 1 - i) & 0xFF;
                crc_add(&crcSpec, &crcSession, &b, 1);
            }
            break;
        }

        case AppSpecValueType_Float:
        {
            static const uint16_t word = 1;
            bool be = *(const char *)&word == 0;

            auto value = literal.FloatValue();
            auto data = reinterpret_cast<const uint8_t *>(&value);
            for (unsigned i = 0; i < sizeof value; i++)
            {
                auto b = data[be ? i : sizeof value - 1 - i];
                crc_add(&crcSpec, &crcSession, &b, 1);
            }
            break;
        }

        case AppSpecValueType_String:
        {
            const auto &s = literal.StringValue();
            crc_add(&crcSpec, &crcSession, s.c_str(), s.size());
        }
    }
}
