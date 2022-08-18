//
// Asp application specification generator implementation.
//

#include "generator.h"
#include "symbol.hpp"
#include "grammar.hpp"
#include <iostream>

using namespace std;

Generator::Generator
    (ostream &errorStream,
     SymbolTable &symbolTable,
     const string &baseFileName) :
    errorStream(errorStream),
    symbolTable(symbolTable),
    baseFileName(baseFileName)
{
    // Reserve main module symbol.
    symbolTable.Symbol("!");
}

Generator::~Generator()
{
}

unsigned Generator::ErrorCount() const
{
    return errorCount;
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
    if (result) \
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
    // TODO: Implement inclusion of other specs.
    return 0;
}

DEFINE_ACTION
    (MakeFunction, NonTerminal *,
     Token *, nameToken, ParameterList *, parameterList,
     Token *, internalNameToken)
{
    // Replace any previous function definition having the same name with
    // this latter one.
    auto findIter = functionDefinitions.find
        (FunctionDefinition(nameToken->s, "", 0));
    if (findIter != functionDefinitions.end())
    {
        cout << "Warning: function " << nameToken->s << " redefined" << endl;
        functionDefinitions.erase(findIter);
    }

    functionDefinitions.emplace
        (nameToken->s, internalNameToken->s, parameterList);
    checkValueComputed = false;

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
    (MakeParameter, Parameter *, Token *, nameToken)
{
    auto result = new Parameter(*nameToken);
    delete nameToken;
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
