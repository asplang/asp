//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "app.h"
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
    (const string &sourceFileName, const SourceLocation &sourceLocation)
{
    currentSourceFileName = sourceFileName;
    currentSourceLocation = sourceLocation;
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
    (IncludeHeader, NonTerminal *, Token *, includeNameToken)
{
    currentSourceFileName = includeNameToken->s + ".asps";
    currentSourceLocation = includeNameToken->sourceLocation;

    delete includeNameToken;

    return 0;
}

DEFINE_ACTION
    (MakeAssignment, NonTerminal *,
     Token *, nameToken, Literal *, value)
{
    if (CheckReservedNameError(nameToken->s))
        return 0;

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

    return 0;
}

DEFINE_ACTION
    (MakeFunction, NonTerminal *,
     Token *, nameToken, ParameterList *, parameterList,
     Token *, internalNameToken)
{
    if (CheckReservedNameError(nameToken->s))
        return 0;

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
            (*nameToken, *internalNameToken, parameterList));
    checkValueComputed = false;

    currentSourceLocation = nameToken->sourceLocation;

    delete nameToken;
    delete internalNameToken;

    return 0;
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
    (MakeParameter, Parameter *,
     Token *, nameToken, Literal *, defaultValue)
{
    auto result = new Parameter(*nameToken, defaultValue);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeGroupParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken, 0, true);
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
    errorStream
        << currentSourceLocation.fileName << ':'
        << currentSourceLocation.line << ':'
        << currentSourceLocation.column << ": Error: "
        << error << endl;
    errorCount++;
}
