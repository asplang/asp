//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "appspec.h"
#include "data.h"
#include "crc.h"
#include <iterator>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <utility>

using namespace std;

static string EngineAppSpec1EngineVersion = "1.2.3.0";
static string EngineAppSpec1EngineVersionHex = "0x01020300";
static string EngineAppSpec1EngineVersionGoodCheck =
    string("#if ASP_VERSION >= ") + EngineAppSpec1EngineVersionHex;
static string EngineAppSpec1EngineVersionBadCheck =
    string("#if ASP_VERSION < ") + EngineAppSpec1EngineVersionHex;
static const string
    CheckValueFeatureBitsPrefix = "f",
    CheckValueModulePrefix = ".",
    CheckValueVariablePrefix = "\v",
    CheckValueFunctionPrefix = "\f",
    CheckValueParameterPrefix = "(",
    CheckValueClassPrefix = "c";

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

typedef struct
{
    set<string> &mainNames;
    map<string, list<string> > &parameterNamesByFunction;
} AssignSymbolsArgument;

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

    // Assign symbols to variable, function, and class names first, then to
    // parameter names, writing each name only once, in order of assigned
    // symbol.
    set<string> mainNames;
    map<string, list<string> > parameterNamesByFunction;
    AssignSymbolsArgument assignSymbolsArg =
        {mainNames, parameterNamesByFunction};
    ForEachModuleAndDefinition
        (nullptr, &Generator::AssignDefinitionSymbols, nullptr, nullptr,
         &assignSymbolsArg);
    for (const auto &mainName: mainNames)
    {
        if (symbolTable.IsDefined(mainName))
            continue;

        lastLocalSymbol = symbolTable.Symbol(mainName);
        os << mainName << delim;
    }
    for (const auto &parameterNameByFunctionEntry: parameterNamesByFunction)
    {
        const auto &parameterNames = parameterNameByFunctionEntry.second;
        for (const auto &parameterName: parameterNames)
        {
            if (symbolTable.IsDefined(parameterName))
                continue;

            lastLocalSymbol = symbolTable.Symbol(parameterName);
            os << parameterName << delim;
        }
    }

    #ifdef ASP_FEATURE_CLASS

    // Assign symbols to qualified names. Note that these are not written to
    // the compiler spec, but are used in the generated C code.
    ForEachModuleAndDefinition
        (nullptr, &Generator::AssignQualifiedNameSymbol, nullptr, nullptr,
         &symbolTable);

    #endif

    symbolsAssigned = true;
}

void Generator::AssignDefinitionSymbols
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto args = reinterpret_cast<AssignSymbolsArgument *>(arg);
    auto &mainNames = args->mainNames;
    auto &parameterNamesByFunction = args->parameterNamesByFunction;

    // Add the variable, function, or class name.
    mainNames.insert(localName);

    const auto functionDefinition =
        dynamic_cast<const FunctionDefinition *>(&definition);
    if (functionDefinition == nullptr)
        return;

    // Add the list of parameter names for the function.
    auto &parameterList = parameterNamesByFunction.insert
        (make_pair(qualifiedName, list<string>())).first->second;
    const auto &parameters = functionDefinition->Parameters();
    for (auto parameterIter = parameters.ParametersBegin();
         parameterIter != parameters.ParametersEnd();
         parameterIter++)
    {
        const auto &parameter = **parameterIter;
        const auto &parameterName = parameter.Name();

        parameterList.push_back(parameterName);
    }
}

#ifdef ASP_FEATURE_CLASS

void Generator::AssignQualifiedNameSymbol
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto symbolTable = reinterpret_cast<SymbolTable *>(arg);

    if (qualifiedName == localName)
        return;

    symbolTable->Symbol(qualifiedName);
}

#endif

typedef struct
{
    ostream &os;
    set<string> &functionInternalNames;
} WriteApplicationHeaderArgument;

void Generator::WriteApplicationHeader(ostream &os) const
{
    if (!symbolsAssigned)
        throw string("Internal error: Symbol not assigned");

    // Write initial header code.
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
    for (auto iter = symbolTable.Begin();
         iter != symbolTable.End();
         iter++)
    {
        const auto &name = iter->first;
        const auto &symbol = iter->second;
        if (symbol > lastLocalSymbol)
            continue;
        os
            << "#define ASP_APP_" << variableBaseName << "_SYM_" << name
            << ' ' << symbol << '\n';
    }

    // Write application function declarations.
    set<string> functionInternalNames;
    WriteApplicationHeaderArgument arg = {os, functionInternalNames};
    ForEachModuleAndDefinition
        (nullptr,
         &Generator::WriteApplicationHeaderDefinition, nullptr,
         nullptr, &arg);

    // Write final header code.
    os
        << "\n"
           "#ifdef __cplusplus\n"
           "}\n"
           "#endif\n\n"
           "#endif\n";
}

void Generator::WriteApplicationHeaderDefinition
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto args = reinterpret_cast<WriteApplicationHeaderArgument *>(arg);
    auto &os = args->os;
    auto &functionInternalNames = args->functionInternalNames;

    const auto functionDefinition =
        dynamic_cast<const FunctionDefinition *>(&definition);
    if (functionDefinition == nullptr)
        return;

    // Prevent writing duplicate declarations.
    const auto &functionInternalName = functionDefinition->InternalName();
    auto insertResult = functionInternalNames.insert(functionInternalName);
    if (!insertResult.second)
        return;

    os << '\n';
    if (functionDefinition->IsLibraryInterface())
        os << "ASP_LIB_API ";
    os
        << "AspRunResult " << functionInternalName << "\n"
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
                << (parameter.IsTupleGroup() ?
                   "tuple" : "dictionary")
                << " group */";
        os << '\n';
    }

    if (parameters.ParametersEmpty())
        os << ' ';
    else
        os << "     ";

    os << "AspDataEntry **returnValue);\n";
}

typedef struct
{
    ostream &os;
    unsigned &specByteCount;
} WriteApplicationAppSpecCodeArgument;

void Generator::WriteApplicationCode(ostream &os) const
{
    if (!symbolsAssigned)
        throw string("Internal error: Symbol not assigned");

    // Write initial code.
    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#include \"" << fileBaseName << ".h\"\n"
           "#include <stdint.h>\n";

    // Write a minimum engine version check if applicable.
    if (engineAppSpecVersion >= 1u)
    {
        os
            << "\n" << EngineAppSpec1EngineVersionBadCheck << "\n"
               "#error Asp engine must be version "
            << EngineAppSpec1EngineVersion << " or greater\n"
            << "#endif\n";
    }

    // Write the dispatch function.
    os
        << "\nstatic AspRunResult AspDispatch_" << variableBaseName
        << "\n    (AspEngine *engine,\n";
    if (engineAppSpecVersion == 0)
        os << "     " << EngineAppSpec1EngineVersionGoodCheck << '\n';
    os << "     int32_t moduleSymbol,";
    if (engineAppSpecVersion == 0)
        os << "\n     #endif\n    ";
    os
        << " int32_t functionSymbol,\n"
           "     AspDataEntry *ns, AspDataEntry **returnValue)\n"
           "{\n";
    if (engineAppSpecVersion == 0)
        os << "    " << EngineAppSpec1EngineVersionGoodCheck << '\n';
    os
        << "    switch (moduleSymbol)\n"
           "    {\n";

    // Write application function declarations.
    ForEachModuleAndDefinition
        (&Generator::WriteApplicationDispatchCodeModuleStart,
         &Generator::WriteApplicationDispatchCodeDefinition,
         nullptr,
         &Generator::WriteApplicationDispatchCodeModuleEnd,
         &os);

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

    WriteApplicationAppSpecCodeArgument arg = {os, specByteCount};
    ForEachModuleAndDefinition
        (&Generator::WriteApplicationAppSpecCodeModule,
         &Generator::WriteApplicationAppSpecCodeDefinitionStart,
         &Generator::WriteApplicationAppSpecCodeDefinitionEnd,
         nullptr,
         &arg);

    // Write application specification final code.
    {
        auto oldFlags = os.flags();
        auto oldFill = os.fill();

        os
            << ",\n    " << specByteCount
            << hex << uppercase << setprecision(4) << setfill('0')
            << ", 0x" << setw(8) << CheckValue() << dec
            << ", AspDispatch_" << variableBaseName << "\n"
               "};\n";

        os.flags(oldFlags);
        os.fill(oldFill);
    }
}

void Generator::WriteApplicationDispatchCodeModuleStart
    (const set<string> &key, const string &name, void *arg) const
{
    auto &os = *reinterpret_cast<ostream *>(arg);

    auto moduleSymbol = -moduleIdTable.Symbol(name);

    os << "        case " << moduleSymbol << ":\n";
    if (engineAppSpecVersion == 0)
        os << "    #endif\n";
    os
        << "            switch (functionSymbol)\n"
           "            {\n"
           "                default:\n"
           "                    break;\n";
}

void Generator::WriteApplicationDispatchCodeModuleEnd
    (const set<string> &key, const string &name, void *arg) const
{
    auto &os = *reinterpret_cast<ostream *>(arg);

    os << "            }\n";
    if (engineAppSpecVersion == 0)
        os << "    " << EngineAppSpec1EngineVersionGoodCheck << '\n';
    os << "            break;\n";
}

void Generator::WriteApplicationDispatchCodeDefinition
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto &os = *reinterpret_cast<ostream *>(arg);

    const auto functionDefinition =
        dynamic_cast<const FunctionDefinition *>(&definition);
    if (functionDefinition == nullptr)
        return;

    auto symbol = symbolTable.Symbol(qualifiedName);

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
                << "                        return _"
                << parameterName << ".result;\n";
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

void Generator::WriteApplicationAppSpecCodeModule
    (const set<string> &key, const string &name, void *arg) const
{
    auto args = reinterpret_cast<WriteApplicationAppSpecCodeArgument *>(arg);
    auto &os = args->os;
    auto &specByteCount = args->specByteCount;

    // Write a module entry if applicable.
    if (name.empty())
        return;

    os << "\n    \"";

    // Write the module entry prefix.
    WriteStringEscapedHex(os, static_cast<uint8_t>(AppSpecPrefix_Module));
    specByteCount++;

    os << '"';
}

void Generator::WriteApplicationAppSpecCodeDefinitionStart
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto args = reinterpret_cast<WriteApplicationAppSpecCodeArgument *>(arg);
    auto &os = args->os;
    auto &specByteCount = args->specByteCount;

    const auto assignment =
        dynamic_cast<const Assignment *>(&definition);
    const auto functionDefinition =
        dynamic_cast<const FunctionDefinition *>(&definition);
    #ifdef ASP_FEATURE_CLASS
    const auto classDefinition =
        dynamic_cast<const ClassDefinition *>(&definition);
    #endif

    // Skip symbol assignments when writing engine app spec
    // version 1 and greater.
    if (engineAppSpecVersion >= 1u && assignment != nullptr &&
        assignment->Value() == nullptr)
        return;

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
            auto nameSymbol = symbolTable.Symbol(localName);
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

        // Write the function entry prefix or 1-byte parameter count. If the
        // parameter count is too big, a standard prefix is written, and the
        // parameter count is written later.
        auto parameterCount = parameters.ParametersSize();
        WriteStringEscapedHex
            (os,
             static_cast<uint8_t>
                (parameterCount >
                 static_cast<size_t>(AppSpecPrefix_MaxFunctionParameterCount) ?
                 AppSpecPrefix_Function : parameterCount));
        specByteCount++;

        // Write the function's symbol(s) if applicable.
        bool writeQualifiedSymbol =
            engineAppSpecVersion >= 2u && qualifiedName != localName;
        if (engineAppSpecVersion >= 1u)
        {
            // Prepare to write the function's name symbol.
            auto nameSymbol = symbolTable.Symbol(localName);
            auto word = *reinterpret_cast<uint32_t *>(&nameSymbol);

            // Add a flag indicating whether the function's qualified symbol
            // will also be written.
            uint32_t flags = 0;
            #ifdef ASP_FEATURE_CLASS
            if (writeQualifiedSymbol)
                flags |= AppSpecFunctionEntryFlag_Qualified;
            #endif
            word |= flags << AspWordBitSize;

            // Write the function name symbol and flags.
            WriteStringEscapedHex(os, *reinterpret_cast<uint32_t *>(&word));
            specByteCount += sizeof nameSymbol;
        }

        // Write the 4-byte parameter count if it was not written as the entry
        // prefix.
        if (parameterCount >
            static_cast<size_t>(AppSpecPrefix_MaxFunctionParameterCount))
        {
            WriteStringEscapedHex
                (os, static_cast<uint32_t>(parameterCount));
            specByteCount += 4;
        }

        // Write the function's qualified symbol if applicable.
        if (writeQualifiedSymbol)
        {
            auto functionSymbol = symbolTable.Symbol(qualifiedName);
            WriteStringEscapedHex
                (os, *reinterpret_cast<uint32_t *>(&functionSymbol));
            specByteCount += sizeof functionSymbol;
        }

        // Write the function's parameter specifications.
        for (auto parameterIter = parameters.ParametersBegin();
             parameterIter != parameters.ParametersEnd();
             parameterIter++)
        {
            const auto &parameter = **parameterIter;

            // Prepare to write the parameter symbol.
            auto parameterSymbol = symbolTable.Symbol(parameter.Name());
            auto word = *reinterpret_cast<uint32_t *>(&parameterSymbol);

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
    #ifdef ASP_FEATURE_CLASS
    else if (classDefinition != nullptr)
    {
        // Write the class start/end entry prefix.
        WriteStringEscapedHex(os, static_cast<uint8_t>(AppSpecPrefix_Class));
        specByteCount++;

        // Prepare to write the class symbol.
        auto nameSymbol = symbolTable.Symbol(localName);
        auto word = *reinterpret_cast<uint32_t *>(&nameSymbol);

        // Add any applicable flags to the class symbol word.
        uint32_t entryType = AppSpecClassEntryType_Start;
        word |= entryType << AspWordBitSize;

        // Write the class symbol and flags.
        WriteStringEscapedHex(os, word);
        specByteCount += sizeof word;
    }
    #endif

    os << '"';
}

void Generator::WriteApplicationAppSpecCodeDefinitionEnd
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto args = reinterpret_cast<WriteApplicationAppSpecCodeArgument *>(arg);
    auto &os = args->os;
    auto &specByteCount = args->specByteCount;

    #ifdef ASP_FEATURE_CLASS

    const auto classDefinition =
        dynamic_cast<const ClassDefinition *>(&definition);
    if (classDefinition == 0)
        return;

    os << "\n    \"";

    // Write the class start/end entry prefix.
    WriteStringEscapedHex(os, static_cast<uint8_t>(AppSpecPrefix_Class));
    specByteCount++;

    // Write a single byte that indicates the end of a class definition.
    WriteStringEscapedHex(os, static_cast<uint8_t>(0));
    specByteCount++;

    os << '"';

    #endif
}

uint32_t Generator::CheckValue() const
{
    if (!finalized)
        throw string("Internal error: Check value not computed");
    return checkValue;
}

typedef struct ComputeCheckValueArgument
{
    const crc_spec &crcSpec;
    crc_session_t &crcSession;
} ComputeCheckValueArgument;

uint32_t Generator::ComputeCheckValue() const
{
    // Use CRC-32/ISO-HDLC for computing a check value.
    const auto crcSpec = crc_make_spec
        (32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF);
    crc_session_t crcSession;
    crc_start(&crcSpec, &crcSession);

    // Contribute the feature set to the check value if applicable.
    if (featureBits != 0)
    {
        crc_add
            (&crcSpec, &crcSession,
             CheckValueFeatureBitsPrefix.c_str(),
             static_cast<unsigned>(CheckValueFeatureBitsPrefix.size()));
        crc_add(&crcSpec, &crcSession, &featureBits, 1);
    }

    ComputeCheckValueArgument arg = {crcSpec, crcSession};
    ForEachModuleAndDefinition
        (&Generator::ComputeCheckValueModule,
         &Generator::ComputeCheckValueDefinition,
         nullptr, nullptr, &arg);

    return static_cast<uint32_t>(crc_finish(&crcSpec, &crcSession));
}

void Generator::ComputeCheckValueModule
    (const set<string> &key, const string &name, void *arg) const
{
    auto args = reinterpret_cast<ComputeCheckValueArgument *>(arg);
    const auto &crcSpec = args->crcSpec;
    auto &crcSession = args->crcSession;

    // For imported modules, contribute the import names that make up the
    // module key, each terminated by a null byte.
    if (!key.empty())
    {
        crc_add
            (&crcSpec, &crcSession,
             CheckValueModulePrefix.c_str(),
             static_cast<unsigned>(CheckValueModulePrefix.size()));
    }
    for (const auto &importName: key)
    {
        crc_add
            (&crcSpec, &crcSession,
             importName.c_str(), static_cast<unsigned>(importName.size()));
        crc_add(&crcSpec, &crcSession, "", 1);
    }
}

void Generator::ComputeCheckValueDefinition
    (const string &localName, const string &qualifiedName,
     const SourceElement &definition, void *arg) const
{
    auto args = reinterpret_cast<ComputeCheckValueArgument *>(arg);
    const auto &crcSpec = args->crcSpec;
    auto &crcSession = args->crcSession;

    const auto assignment =
        dynamic_cast<const Assignment *>(&definition);
    const auto functionDefinition =
        dynamic_cast<const FunctionDefinition *>(&definition);
    #ifdef ASP_FEATURE_CLASS
    const auto classDefinition =
        dynamic_cast<const ClassDefinition *>(&definition);
    #endif

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
             localName.c_str(), static_cast<unsigned>(localName.size()));

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
             localName.c_str(), static_cast<unsigned>(localName.size()));

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
                 static_cast<unsigned>(CheckValueParameterPrefix.size()));
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
    #ifdef ASP_FEATURE_CLASS
    else if (classDefinition != nullptr)
    {
        // Contribute the class name.
        crc_add
            (&crcSpec, &crcSession,
             CheckValueClassPrefix.c_str(),
             static_cast<unsigned>(CheckValueClassPrefix.size()));
        crc_add
            (&crcSpec, &crcSession,
             localName.c_str(), static_cast<unsigned>(localName.size()));

    }
    #endif
    else
        throw string("Internal error: Unknown definition type");
}

void Generator::ForEachModuleAndDefinition
    (void (Generator::*moduleStartAction)
        (const set<string> &key, const string &name, void *arg) const,
     void (Generator::*definitionStartAction)
        (const string &localName, const string &qualifiedName,
         const SourceElement &definition, void *) const,
     void (Generator::*definitionEndAction)
        (const string &localName, const string &qualifiedName,
         const SourceElement &definition, void *) const,
     void (Generator::*moduleEndAction)
        (const set<string> &key, const string &name, void *arg) const,
     void *arg) const
{
    // Visit each module and its definitions.
    for (const auto &moduleEntry: definitionsByModuleKey)
    {
        const auto &moduleKey = moduleEntry.first;
        const auto &moduleName = moduleEntry.second.moduleName;

        // Perform the specified module start action.
        if (moduleStartAction != nullptr)
            (this->*moduleStartAction)(moduleKey, moduleName, arg);

        // Visit each definition and, for structured ones, any definitions
        // contained within recursively.
        deque<pair<shared_ptr<DefinitionMap>, DefinitionMap::iterator> >
            navigationStack;
        auto definitions = moduleEntry.second.definitions;
        navigationStack.push_back
            (make_pair(definitions, definitions->begin()));
        while (!navigationStack.empty())
        {
            auto navigationStackSize = navigationStack.size();

            // Determine the prefix for forming qualified names (i.e., names
            // defined inside (potentially nested) structure definitions, e.g.,
            // classes).
            string namePrefix;
            for (auto navigationStackIter = navigationStack.cbegin();
                 navigationStackIter != prev(navigationStack.cend());
                 navigationStackIter++)
            {
                if (!namePrefix.empty())
                    namePrefix += ':';
                auto nameIter(navigationStackIter->second);
                if (navigationStackIter != prev(navigationStack.end()))
                    nameIter--;
                namePrefix += nameIter->first;
            }

            // Visit each definition within the current module or structured
            // definition.
            auto &stackTop = navigationStack.back();
            for (auto &definitionIter = stackTop.second;
                 definitionIter != stackTop.first->end() &&
                 navigationStack.size() == navigationStackSize;
                 definitionIter++)
            {
                const auto &definitionEntry = *definitionIter;
                const auto &name = definitionEntry.first;
                const auto &definition = definitionEntry.second.get();

                if (definitionStartAction != nullptr)
                {
                    auto qualifiedName =
                        namePrefix + (namePrefix.empty() ? "" : ":") + name;

                    // Perform the specified definition start action.
                    (this->*definitionStartAction)
                        (name, qualifiedName, *definition, arg);
                }

                #ifdef ASP_FEATURE_CLASS

                const auto classDefinition =
                    dynamic_cast<const ClassDefinition *>(definition);
                if (classDefinition != nullptr)
                {
                    // Prepare to process the definitions within the class.
                    auto definitions = classDefinition->definitions;
                    navigationStack.push_back
                        (make_pair(definitions, definitions->begin()));
                }

                #endif
            }

            if (navigationStack.size() == navigationStackSize)
            {
                // Return to the enveloping module or structured definition.
                navigationStack.pop_back();

                if (definitionEndAction != nullptr && !navigationStack.empty())
                {
                    const auto &stackTop = navigationStack.back();
                    const auto definitionIter = prev(stackTop.second);
                    const auto &definitionEntry = *definitionIter;
                    const auto &name = definitionEntry.first;
                    const auto &definition = definitionEntry.second.get();

                    // Perform the specified definition end action.
                    (this->*definitionEndAction)
                        (name, namePrefix, *definition, arg);
                }
            }
        }

        // Perform the specified module end action.
        if (moduleEndAction != nullptr)
            (this->*moduleEndAction)(moduleKey, moduleName, arg);
    }
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
            WriteStringEscapedHex(os, static_cast<uint8_t>(value));
            (*specByteCount)++;
            break;
        }

        case AppSpecValueType_Integer:
        {
            int32_t value = literal.IntegerValue();
            WriteStringEscapedHex(os, *reinterpret_cast<uint32_t *>(&value));
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
            auto valueSize = static_cast<uint32_t>(value.size());
            WriteStringEscapedHex(os, valueSize);
            for (unsigned i = 0; i < valueSize; i++)
                WriteStringEscapedHex(os, static_cast<uint8_t>(value[i]));
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
                uint8_t b = uValue >> 8 * (sizeof value - 1 - i) & 0xFF;
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
