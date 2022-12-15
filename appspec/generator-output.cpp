//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "symbol.hpp"
#include "symbols.h"
#include "crc.h"
#include <iomanip>
#include <set>

using namespace std;

static const uint32_t ParameterFlag_HasDefault = 0x10000000;
static const uint32_t ParameterFlag_IsGroup    = 0x20000000;

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

    os << hex << setprecision(2) << setfill('0');
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
    // Write the specification's check value.
    os.write("AspS", 4);
    Write(os, CheckValue());

    // Reserve symbols used in the system module.
    symbolTable.Symbol(AspSystemModuleName);
    symbolTable.Symbol(AspSystemArgumentsName);

    // Assign symbols, function names first, then parameter names,
    // writing each name only once, in order of assigned symbol.
    set<int> writtenSymbols;
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = *iter;

        auto functionName = functionDefinition.Name();
        auto symbol = symbolTable.Symbol(functionName);

        auto writtenIter = writtenSymbols.find(symbol);
        if (writtenIter == writtenSymbols.end())
        {
            os << functionName << '\n';
            writtenSymbols.insert(symbol);
        }
    }
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = *iter;

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            const auto &parameterName = parameter.Name();
            auto parameterSymbol = symbolTable.Symbol(parameterName);

            auto writtenIter = writtenSymbols.find(parameterSymbol);
            if (writtenIter == writtenSymbols.end())
            {
                os << parameterName << '\n';
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
        auto &functionDefinition = *iter;

        os
            << "\nAspRunResult " << functionDefinition.InternalName() << "\n"
               "    (AspEngine *,";
        if (!functionDefinition.Parameters().ParametersEmpty())
            os << "\n";

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            os << "     AspDataEntry *" << parameter.Name() << ',';
            if (parameter.IsGroup())
                os << " /* group */";
            os << '\n';
        }

        if (functionDefinition.Parameters().ParametersEmpty())
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
    os
        << "/*** AUTO-GENERATED; DO NOT EDIT ***/\n\n"
           "#include \"" << baseFileName << ".h\"\n"
           "#include <stdint.h>\n";

    // Write the dispatch function.
    os
        << "\nstatic AspRunResult AspDispatch_" << baseFileName
        << "\n    (AspEngine *engine, int32_t symbol, AspDataEntry *ns,\n"
           "     AspDataEntry **returnValue)\n"
           "{\n"
           "    switch (symbol)\n"
           "    {\n";
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = *iter;

        auto symbol = symbolTable.Symbol(functionDefinition.Name());

        os
            << "        case " << symbol << ":\n"
            << "        {\n";

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            const auto &parameterName = parameter.Name();
            int32_t parameterSymbol = symbolTable.Symbol(parameterName);

            if (parameter.IsGroup())
            {
                os
                    << "            AspParameterResult " << parameterName
                    << " = AspGroupParameterValue(engine, ns, "
                    << parameterSymbol << ");\n"
                    << "            if (" << parameterName
                    << ".result != AspRunResult_OK)\n"
                    << "                return " << parameterName
                    << ".result;\n";
            }
            else
            {
                os
                    << "            AspDataEntry *" << parameterName
                    << " = AspParameterValue(engine, ns, "
                    << parameterSymbol << ");\n";
            }
        }

        os
            << "            return " << functionDefinition.InternalName()
            << "(engine, ";

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            os << parameter.Name();
            if (parameter.IsGroup())
                os << ".value";
            os << ", ";
        }

        os
            << "returnValue);\n"
               "        }\n";
    }
    os
        << "    }\n"
           "    return AspRunResult_UndefinedAppFunction;\n"
           "}\n";

    // Write the application specification structure.
    os
        << "\nAspAppSpec AspAppSpec_" << baseFileName << " =\n"
           "{";
    unsigned specByteCount = 0;
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = *iter;

        os << "\n    \"";
        auto parameterCount = functionDefinition.Parameters().ParametersSize();
        WriteStringEscapedHex(os, static_cast<uint8_t>(parameterCount));
        specByteCount++;

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            int32_t parameterSymbol = symbolTable.Symbol(parameter.Name());
            uint32_t word = *reinterpret_cast<uint32_t *>(&parameterSymbol);

            const auto &defaultValue = parameter.DefaultValue();
            if (defaultValue != 0)
                word |= ParameterFlag_HasDefault;
            if (parameter.IsGroup())
                word |= ParameterFlag_IsGroup;

            WriteStringEscapedHex(os, word);
            specByteCount += sizeof word;

            if (defaultValue != 0)
            {
                auto valueType = defaultValue->GetType();
                WriteStringEscapedHex(os, static_cast<uint8_t>(valueType));
                specByteCount++;

                switch (valueType)
                {
                    case Literal::Type::Boolean:
                    {
                        auto value = defaultValue->BooleanValue();
                        WriteStringEscapedHex
                            (os, static_cast<uint8_t>(value));
                        specByteCount++;
                        break;
                    }

                    case Literal::Type::Integer:
                    {
                        auto value = defaultValue->IntegerValue();
                        WriteStringEscapedHex
                            (os, *reinterpret_cast<uint32_t *>(&value));
                        specByteCount += sizeof value;
                        break;
                    }

                    case Literal::Type::Float:
                    {
                        static const uint16_t word = 1;
                        bool be = *(const char *)&word == 0;

                        auto value = defaultValue->FloatValue();
                        auto data = reinterpret_cast<const uint8_t *>(&value);
                        for (unsigned i = 0; i < sizeof value; i++)
                        {
                            auto b = data[be ? i : sizeof value - 1 - i];
                            WriteStringEscapedHex(os, b);
                        }
                        specByteCount += sizeof value;
                        break;
                    }

                    case Literal::Type::String:
                    {
                        auto value = defaultValue->StringValue();
                        uint32_t valueSize = static_cast<uint32_t>
                            (value.size());
                        WriteStringEscapedHex(os, valueSize);
                        for (unsigned i = 0; i < valueSize; i++)
                            WriteStringEscapedHex
                                (os, static_cast<uint8_t>(value[i]));
                        specByteCount += sizeof(valueSize) + valueSize;
                        break;
                    }
                }
            }
        }
        os << '"';
    }

    {
        auto oldFlags = os.flags();
        auto oldFill = os.fill();

        os
            << ",\n    " << specByteCount
            << hex << setprecision(4) << setfill('0')
            << ", 0x" << setw(4) << CheckValue() << dec
            << ", AspDispatch_" << baseFileName << "\n"
               "};\n";

        os.flags(oldFlags);
        os.fill(oldFill);
    }
}

uint32_t Generator::CheckValue()
{
    if (!checkValueComputed)
    {
        ComputeCheckValue();
        checkValueComputed = true;
    }
    return checkValue;
}

void Generator::ComputeCheckValue()
{
    // Use CRC-32/ISO-HDLC for computing a check value.
    auto spec = crc_make_spec
        (32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF);
    crc_session session;
    crc_start(&spec, &session);

    // Compute a check value based on the signature of each function.
    static const string
        CheckValueFunctionPrefix = "\f",
        CheckValueParameterPrefix = "(";
    for (auto iter = functionDefinitions.begin();
         iter != functionDefinitions.end(); iter++)
    {
        auto &functionDefinition = *iter;

        const auto &functionName = functionDefinition.Name();

        // Contribute the function name.
        crc_add
            (&spec, &session,
             CheckValueFunctionPrefix.c_str(),
             static_cast<unsigned>(CheckValueFunctionPrefix.size()));
        crc_add
            (&spec, &session,
             functionName.c_str(),
             static_cast<unsigned>(functionName.size()));

        for (auto parameterIter =
             functionDefinition.Parameters().ParametersBegin();
             parameterIter != functionDefinition.Parameters().ParametersEnd();
             parameterIter++)
        {
            auto &parameter = **parameterIter;

            const auto &parameterName = parameter.Name();

            // Contribute the parameter name.
            crc_add
                (&spec, &session,
                 CheckValueParameterPrefix.c_str(),
                 static_cast<unsigned>(CheckValueParameterPrefix.size()));
            crc_add
                (&spec, &session,
                 parameterName.c_str(),
                 static_cast<unsigned>(parameterName.size()));

            // Contribute the default value if present.
            const auto &defaultValue = parameter.DefaultValue();
            if (defaultValue != 0)
            {
                auto valueType = defaultValue->GetType();
                auto b = static_cast<uint8_t>(valueType);
                crc_add(&spec, &session, &b, 1);

                switch (valueType)
                {
                    case Literal::Type::Boolean:
                    {
                        uint8_t b = defaultValue->BooleanValue() ? 1 : 0;
                        crc_add(&spec, &session, &b, 1);
                        break;
                    }

                    case Literal::Type::Integer:
                    {
                        auto value = defaultValue->IntegerValue();
                        auto uValue = *reinterpret_cast<uint32_t *>(&value);
                        for (unsigned i = 0; i < sizeof value; i++)
                        {
                            uint8_t b =
                                uValue >> 8 * (sizeof value - 1 - i) & 0xFF;
                            crc_add(&spec, &session, &b, 1);
                        }
                        break;
                    }

                    case Literal::Type::Float:
                    {
                        static const uint16_t word = 1;
                        bool be = *(const char *)&word == 0;

                        auto value = defaultValue->FloatValue();
                        auto data = reinterpret_cast<const uint8_t *>(&value);
                        for (unsigned i = 0; i < sizeof value; i++)
                        {
                            auto b = data[be ? i : sizeof value - 1 - i];
                            crc_add(&spec, &session, &b, 1);
                        }
                        break;
                    }

                    case Literal::Type::String:
                    {
                        const auto &s = defaultValue->StringValue();
                        crc_add(&spec, &session, s.c_str(), s.size());
                    }
                }
            }
        }
    }
    checkValue = static_cast<uint32_t>(crc_finish(&spec, &session));
}
