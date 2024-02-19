//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "app.h"
#include "function.hpp"
#include "symbol.hpp"
#include "symbols.h"
#include "grammar.hpp"
#include <iostream>
#include <sstream>

using namespace std;

Generator::Generator
    (ostream &errorStream,
     SymbolTable &symbolTable,
     const string &baseFileName) :
    errorStream(errorStream),
    symbolTable(symbolTable),
    baseFileName(baseFileName)
{
}

Generator::~Generator()
{
    for (auto iter = definitions.begin(); iter != definitions.end(); iter++)
        delete iter->second;
}

unsigned Generator::ErrorCount() const
{
    return errorCount;
}

void Generator::CurrentSource
    (const string &sourceFileName,
     bool newFile, bool isLibrary,
     const SourceLocation &sourceLocation)
{
    this->newFile = newFile;
    this->isLibrary = isLibrary;
    currentSourceFileName = sourceFileName;
    currentSourceLocation = sourceLocation;
}

bool Generator::IsLibrary() const
{
    return isLibrary;
}

const string &Generator::CurrentSourceFileName() const
{
    return currentSourceFileName;
}

SourceLocation Generator::CurrentSourceLocation() const
{
    return currentSourceLocation;
}

#define DEFINE_ACTION(...) DEFINE_ACTION_N(__VA_ARGS__, \
    DEFINE_ACTION_3, ~, \
    DEFINE_ACTION_2, ~, \
    DEFINE_ACTION_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_ACTION_N(_1, _2, _3, _4, _5, _6, _7, _8, arg, ...) arg
#define DEFINE_ACTION_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Generator *generator, type param) \
    {DO_ACTION(generator->action(param));} \
    ReturnType Generator::action(type param)
#define DEFINE_ACTION_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2) \
    {DO_ACTION(generator->action(p1, p2));} \
    ReturnType Generator::action \
        (t1 p1, t2 p2)
#define DEFINE_ACTION_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2, t3 p3) \
    {DO_ACTION(generator->action(p1, p2, p3));} \
    ReturnType Generator::action \
        (t1 p1, t2 p2, t3 p3)
#define DO_ACTION(action) \
    auto result = (action); \
    if (result && result->sourceLocation.line != 0) \
        generator->currentSourceLocation = result->sourceLocation; \
    return result;

#define DEFINE_UTIL(...) DEFINE_UTIL_N(__VA_ARGS__, \
    DEFINE_UTIL_3, ~, \
    DEFINE_UTIL_2, ~, \
    DEFINE_UTIL_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_UTIL_N(_1, _2, _3, _4, _5, _6, _7, _8, arg, ...) arg
#define DEFINE_UTIL_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Generator *generator, type param) \
    {generator->action(param);} \
    ReturnType Generator::action(type param)
#define DEFINE_UTIL_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2) \
    {generator->action(p1, p2);} \
    ReturnType Generator::action \
        (t1 p1, t2 p2)
#define DEFINE_UTIL_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Generator *generator, \
        t1 p1, t2 p2, t3 p3) \
    {generator->action(p1, p2, p3);} \
    ReturnType Generator::action \
        (t1 p1, t2 p2, t3 p3)

DEFINE_ACTION
    (DeclareAsLibrary, NonTerminal *, int, _)
{
    // Allow library declaration only as the first statement.
    if (!newFile)
    {
        ReportError("lib must be the first statement");
        return nullptr;
    }

    isLibrary = true;
    return nullptr;
}

DEFINE_ACTION
    (IncludeHeader, NonTerminal *, Token *, includeNameToken)
{
    newFile = false;

    currentSourceFileName = includeNameToken->s + ".asps";
    currentSourceLocation = includeNameToken->sourceLocation;

    delete includeNameToken;

    return nullptr;
}

DEFINE_ACTION
    (MakeAssignment, NonTerminal *,
     Token *, nameToken, Literal *, value)
{
    newFile = false;

    if (CheckReservedNameError(nameToken->s))
        return nullptr;

    // Replace any previous definition having the same name with this
    // latter one.
    auto findIter = definitions.find(nameToken->s);
    if (findIter != definitions.end())
    {
        cout << "Warning: " << nameToken->s << " redefined" << endl;
        delete findIter->second;
        definitions.erase(findIter);
    }

    definitions.emplace
        (nameToken->s, new Assignment(*nameToken, value));
    checkValueComputed = false;

    currentSourceLocation = nameToken->sourceLocation;

    delete nameToken;

    return nullptr;
}

DEFINE_ACTION
    (MakeFunction, NonTerminal *,
     Token *, nameToken, ParameterList *, parameterList,
     Token *, internalNameToken)
{
    newFile = false;

    if (CheckReservedNameError(nameToken->s))
        return nullptr;

    // Ensure the validity of the order of parameter types.
    int position = 1;
    ValidFunctionDefinition validFunctionDefinition;
    for (auto iter = parameterList->ParametersBegin();
         validFunctionDefinition.IsValid() &&
         iter != parameterList->ParametersEnd();
         iter++, position++)
    {
        const auto &parameter = **iter;

        ValidFunctionDefinition::ParameterType type;
        bool typeValid = true;
        switch (parameter.GetType())
        {
            default:
                typeValid = false;
                break;
            case Parameter::Type::Positional:
                type = ValidFunctionDefinition::ParameterType::Positional;
                break;
            case Parameter::Type::TupleGroup:
                type = ValidFunctionDefinition::ParameterType::TupleGroup;
                break;
            case Parameter::Type::DictionaryGroup:
                type = ValidFunctionDefinition::ParameterType::DictionaryGroup;
                break;
        }
        if (!typeValid)
        {
            ReportError("Internal error; unknown parameter type");
            break;
        }

        string error = validFunctionDefinition.AddParameter
            (parameter.Name(), type, parameter.DefaultValue() != nullptr);
        if (!error.empty())
            ReportError(error, parameter);
    }

    // Replace any previous definition having the same name with this
    // latter one.
    auto findIter = definitions.find(nameToken->s);
    if (findIter != definitions.end())
    {
        cout << "Warning: " << nameToken->s << " redefined" << endl;
        delete findIter->second;
        definitions.erase(findIter);
    }

    definitions.emplace
        (nameToken->s,
         new FunctionDefinition
            (*nameToken, isLibrary,
             *internalNameToken, parameterList));
    checkValueComputed = false;

    currentSourceLocation = nameToken->sourceLocation;

    delete nameToken;
    delete internalNameToken;

    return nullptr;
}

DEFINE_ACTION
    (MakeEmptyParameterList, ParameterList *, int, _)
{
    return new ParameterList;
}

DEFINE_ACTION
    (AddParameterToList, ParameterList *,
     ParameterList *, parameterList, Parameter *, parameter)
{
    parameterList->Add(parameter);
    return parameterList;
}

DEFINE_ACTION
    (MakeParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDefaultedParameter, Parameter *,
     Token *, nameToken, Literal *, defaultValue)
{
    auto result = new Parameter(*nameToken, defaultValue);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeTupleGroupParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken, Parameter::Type::TupleGroup);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDictionaryGroupParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken, Parameter::Type::DictionaryGroup);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeLiteral, Literal *, Token *, token)
{
    auto result = new Literal(*token);
    delete token;
    return result;
}

DEFINE_UTIL(FreeNonTerminal, void, NonTerminal *, nt)
{
    delete reinterpret_cast<NonTerminal *>(nt);
}

DEFINE_UTIL(FreeToken, void, Token *, token)
{
    delete token;
}

DEFINE_UTIL(ReportError, void, const char *, error)
{
     ReportError(string(error));
}

bool Generator::CheckReservedNameError(const string &name)
{
    if (name == AspSystemModuleName ||
        name == AspSystemArgumentsName)
    {
        ostringstream oss;
        oss << "Cannot redefine reserved name '" << name << '\'';
        ReportError(oss.str());
        return true;
    }

    return false;
}

void Generator::ReportError(const string &error)
{
    ReportError(error, currentSourceLocation);
}

void Generator::ReportError
    (const string &error, const SourceElement &sourceElement)
{
    ReportError(error, sourceElement.sourceLocation);
}

void Generator::ReportError
    (const string &error, const SourceLocation &sourceLocation)
{
    errorStream
        << sourceLocation.fileName << ':'
        << sourceLocation.line << ':'
        << sourceLocation.column << ": Error: "
        << error << endl;
    errorCount++;
}
