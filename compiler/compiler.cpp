//
// Asp compiler implementation.
//

#include "compiler.h"
#include "grammar.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "instruction.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdint>

using namespace std;

const static string ModuleSuffix = ".asp";

Compiler::Compiler
    (ostream &errorStream,
     SymbolTable &symbolTable, Executable &executable) :
    errorStream(errorStream),
    symbolTable(symbolTable),
    executable(executable),
    topLocation(executable.Insert(new NullInstruction))
{
}

Compiler::~Compiler()
{
}

void Compiler::LoadApplicationSpec(istream &specStream)
{
    // Read and check application spec header.
    char header[4];
    specStream.read(header, 4);
    if (memcmp(header, "AspS", 4) != 0)
        throw string("Invalid format in application spec file");

    // Read application spec check value and store.
    uint32_t checkValue = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        checkValue <<= 8;
        checkValue |= specStream.get();
        if (specStream.eof())
            throw string("Invalid format in application spec file");
    }
    executable.SetCheckValue(checkValue);

    // Define symbols for all names used in the application.
    while (true)
    {
        string name;
        specStream >> name;
        if (specStream.eof())
            break;
        int32_t symbol = symbolTable.Symbol(name);
    }

    // Reserve symbol for system namespace.
    int32_t systemNamespaceSymbol = symbolTable.Symbol();
}

void Compiler::AddModule(const string &moduleName)
{
    // Add the module only if it has never been added before.
    auto iter = moduleNames.find(moduleName);
    if (iter == moduleNames.end())
    {
        int32_t moduleSymbol = symbolTable.Symbol(moduleName);
        moduleNames.insert(moduleName);
        moduleNamesToImport.push_back(moduleName);
    }
}

void Compiler::AddModuleFileName(const string &moduleFileName)
{
    // Ensure the file name includes the proper suffix.
    bool bad = moduleFileName.size() <= ModuleSuffix.size();
    auto suffixIndex = moduleFileName.size() - ModuleSuffix.size();
    bad = !bad && moduleFileName.substr(suffixIndex) != ModuleSuffix;
    if (bad)
    {
        ostringstream oss;
        oss
            << "Module file name '" << moduleFileName
            << "' does not end with '" << ModuleSuffix << '\'';
        throw oss.str();
    }

    // Strip off the suffix and add the module name.
    AddModule(moduleFileName.substr(0, suffixIndex));
}

string Compiler::NextModuleFileName()
{
    string moduleFileName;
    if (moduleNamesToImport.empty())
        currentModuleName.clear();
    else
    {
        currentModuleName = moduleNamesToImport.front();
        currentModuleSymbol = symbolTable.Symbol(currentModuleName);
        moduleNamesToImport.pop_front();
        executable.CurrentModule(currentModuleName);
        moduleFileName = currentModuleName + ModuleSuffix;
    }
    return moduleFileName;
}

unsigned Compiler::ErrorCount() const
{
    return errorCount;
}

void Compiler::Finalize()
{
    // Invoke the top-level module.
    executable.PushLocation(topLocation);
    executable.Insert(new LoadModuleInstruction
        (0, "Load top-level module"));
    executable.Insert(new EndInstruction("End script"));
    executable.PopLocation();

    executable.Finalize();
}

#define DEFINE_ACTION(...) DEFINE_ACTION_N(__VA_ARGS__, \
    DEFINE_ACTION_4, ~, \
    DEFINE_ACTION_3, ~, \
    DEFINE_ACTION_2, ~, \
    DEFINE_ACTION_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_ACTION_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _a, arg, ...) arg
#define DEFINE_ACTION_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Compiler *compiler, type param) \
    {DO_ACTION(compiler->action(param));} \
    ReturnType Compiler::action(type param)
#define DEFINE_ACTION_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2) \
    {DO_ACTION(compiler->action(p1, p2));} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2)
#define DEFINE_ACTION_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2, t3 p3) \
    {DO_ACTION(compiler->action(p1, p2, p3));} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2, t3 p3)
#define DEFINE_ACTION_4(action, ReturnType, t1, p1, t2, p2, t3, p3, t4, p4) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2, t3 p3, t4 p4) \
    {DO_ACTION(compiler->action(p1, p2, p3, p4));} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2, t3 p3, t4 p4)
#define DO_ACTION(action) \
    auto result = (action); \
    if (result) \
        compiler->currentSourceLocation = result->sourceLocation; \
    return result;

#define DEFINE_UTIL(...) DEFINE_UTIL_N(__VA_ARGS__, \
    DEFINE_UTIL_4, ~, \
    DEFINE_UTIL_3, ~, \
    DEFINE_UTIL_2, ~, \
    DEFINE_UTIL_1, ~, ~, ~, ~)(__VA_ARGS__)
#define DEFINE_UTIL_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _a, arg, ...) arg
#define DEFINE_UTIL_1(action, ReturnType, type, param) \
    extern "C" ReturnType action(Compiler *compiler, type param) \
    {compiler->action(param);} \
    ReturnType Compiler::action(type param)
#define DEFINE_UTIL_2(action, ReturnType, t1, p1, t2, p2) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2) \
    {compiler->action(p1, p2);} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2)
#define DEFINE_UTIL_3(action, ReturnType, t1, p1, t2, p2, t3, p3) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2, t3 p3) \
    {compiler->action(p1, p2, p3);} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2, t3 p3)
#define DEFINE_UTIL_4(action, ReturnType, t1, p1, t2, p2, t3, p3, t4, p4) \
    extern "C" ReturnType action(Compiler *compiler, \
        t1 p1, t2 p2, t3 p3, t4 p4) \
    {compiler->action(p1, p2, p3, p4);} \
    ReturnType Compiler::action \
        (t1 p1, t2 p2, t3 p3, t4 p4)

DEFINE_ACTION(MakeModule, NonTerminal *, Block *, module)
{
    try
    {
        auto moduleLocation = executable.Insert(new NullInstruction);
        executable.MarkModuleLocation(currentModuleName, moduleLocation);

        executable.PushLocation(topLocation);
        {
            ostringstream oss;
            oss
                << "Add address of module " << currentModuleName
                << " (" << currentModuleSymbol << ')';
            executable.Insert(new AddModuleInstruction
                (currentModuleSymbol, moduleLocation, oss.str()));
        }
        executable.PopLocation();

        module->Emit(executable);
        executable.Insert(new ExitModuleInstruction("Exit module"));
    }
    catch (const string &e)
    {
        ReportError(e);
    }

    delete module;

    return 0;
}

DEFINE_ACTION(MakeEmptyBlock, Block *, int, _)
{
    return new Block;
}

DEFINE_ACTION
    (AddStatementToBlock, Block *,
     Block *, block, Statement *, statement)
{
    if (statement)
        block->Add(statement);
    return block;
}

DEFINE_ACTION(AssignBlock, Block *, Block *, block)
{
    return block;
}

DEFINE_ACTION
    (MakeMultipleAssignmentStatement, AssignmentStatement *,
     Token *, assignmentToken, Expression *, targetExpression,
     AssignmentStatement *, valueAssignmentStatement)
{
    auto result = new AssignmentStatement
        (*assignmentToken, targetExpression, valueAssignmentStatement);
    delete assignmentToken;
    return result;
}

DEFINE_ACTION
    (MakeSingleAssignmentStatement, AssignmentStatement *,
     Token *, assignmentToken, Expression *, targetExpression,
     Expression *, valueExpression)
{
    auto result = new AssignmentStatement
        (*assignmentToken, targetExpression, valueExpression);
    delete assignmentToken;
    return result;
}

DEFINE_ACTION
    (AssignAssignmentStatement, AssignmentStatement *,
     AssignmentStatement *, assignmentStatement)
{
    return assignmentStatement;
}

DEFINE_ACTION
    (MakeAssignmentStatement, Statement *,
     AssignmentStatement *, assignmentStatement)
{
    return assignmentStatement;
}

DEFINE_ACTION
    (MakeMultipleValueInsertionStatement, InsertionStatement *,
     Token *, insertionToken, InsertionStatement *, insertionStatement,
     Expression *, itemExpression)
{
    auto result = new InsertionStatement
        (*insertionToken, insertionStatement, itemExpression);
    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeMultiplePairInsertionStatement, InsertionStatement *,
     Token *, insertionToken, InsertionStatement *, insertionStatement,
     KeyValuePair *, keyValuePair)
{
    auto result = new InsertionStatement
        (*insertionToken, insertionStatement, keyValuePair);
    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeSingleValueInsertionStatement, InsertionStatement *,
     Token *, insertionToken, Expression *, containerExpression,
     Expression *, itemExpression)
{
    auto result = new InsertionStatement
        (*insertionToken, containerExpression, itemExpression);
    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeSinglePairInsertionStatement, InsertionStatement *,
     Token *, insertionToken, Expression *, containerExpression,
     KeyValuePair *, keyValuePair)
{
    auto result = new InsertionStatement
        (*insertionToken, containerExpression, keyValuePair);
    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeInsertionStatement, Statement *,
     InsertionStatement *, insertionStatement)
{
    return insertionStatement;
}

DEFINE_ACTION
    (MakeExpressionStatement, Statement *,
     Expression *, expression)
{
    return new ExpressionStatement(expression);
}

DEFINE_ACTION
    (MakeImportStatement, Statement *,
     ImportNameList *, moduleNameList)
{
    for (auto iter = moduleNameList->NamesBegin();
         iter != moduleNameList->NamesEnd(); iter++)
    {
        auto importName = *iter;
        AddModule(importName->Name());
    }

    return new ImportStatement(moduleNameList);
}

DEFINE_ACTION
    (MakeFromImportStatement, Statement *,
     ImportName *, moduleName, ImportNameList *, memberNameList)
{
    AddModule(moduleName->Name());

    auto moduleNameList = new ImportNameList;
    moduleNameList->Add(moduleName);
    return new ImportStatement(moduleNameList, memberNameList);
}

DEFINE_ACTION
    (MakeGlobalStatement, Statement *,
     VariableList *, variableList)
{
    return new GlobalStatement(variableList);
}

DEFINE_ACTION
    (MakeLocalStatement, Statement *,
     VariableList *, variableList)
{
    return new LocalStatement(variableList);
}

DEFINE_ACTION
    (MakeDelStatement, Statement *,
     Expression *, expression)
{
    return new DelStatement(expression);
}

DEFINE_ACTION
    (MakeReturnStatement, Statement *,
     Token *, keywordToken, Expression *, expression)
{
    auto result = new ReturnStatement(*keywordToken, expression);
    delete keywordToken;
    return result;
}

DEFINE_ACTION(MakeBreakStatement, Statement *, Token *, token)
{
    auto result = new BreakStatement(*token);
    delete token;
    return result;
}

DEFINE_ACTION(MakeContinueStatement, Statement *, Token *, token)
{
    auto result = new ContinueStatement(*token);
    delete token;
    return result;
}

DEFINE_ACTION(MakePassStatement, Statement *, Token *, token)
{
    auto result = new PassStatement(*token);
    delete token;
    return result;
}

DEFINE_ACTION
    (MakeIfElseIfStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock, IfStatement *, ifStatement)
{
    return new IfStatement(expression, trueBlock, ifStatement);
}

DEFINE_ACTION
    (MakeIfElseStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock, Block *, falseBlock)
{
    return new IfStatement(expression, trueBlock, falseBlock);
}

DEFINE_ACTION
    (MakeIfStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock)
{
    return new IfStatement(expression, trueBlock);
}

DEFINE_ACTION
    (MakeWhileStatement, Statement *,
     Expression *, expression, Block *, trueBlock, Block *, falseBlock)
{
    return new WhileStatement(expression, trueBlock, falseBlock);
}

DEFINE_ACTION
    (MakeForStatement, Statement *,
     VariableList *, variableList, Expression *,expression,
     Block *, trueBlock, Block *, falseBlock)
{
    return new ForStatement
        (variableList, expression, trueBlock, falseBlock);
}

DEFINE_ACTION
    (MakeDefStatement, Statement *,
     Token *, nameToken, ParameterList *, parameterList, Block *, block)
{
    // Ensure defaulted parameters follow positional ones.
    bool positionalParameterAllowed = true;
    unsigned position = 1;
    for (auto iter = parameterList->ParametersBegin();
         iter != parameterList->ParametersEnd(); iter++, position++)
    {
        auto parameter = *iter;

        if (parameter->HasDefault())
            positionalParameterAllowed = false;
        else if (!positionalParameterAllowed)
        {
            ostringstream oss;
            oss
                << "Parameter " << position
                << " with no default follows parameter(s) with default(s)";
            ReportError(oss.str());
        }
    }

    auto result = new DefStatement(*nameToken, parameterList, block);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (AssignStatement, Statement *, Statement *, statement)
{
    return statement;
}

DEFINE_ACTION
    (MakeTernaryExpression, Expression *,
     Token *, operatorToken, Expression *, conditionExpression,
     Expression *, trueExpression, Expression *, falseExpression)
{
    // TODO: Fold constant expressions. See unary expression logic.
    auto result = new TernaryExpression
        (*operatorToken, conditionExpression,
         trueExpression, falseExpression);
    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeBinaryExpression, Expression *,
     Token *, operatorToken,
     Expression *, leftExpression, Expression *, rightExpression)
{
    // TODO: Fold constant expressions. See unary expression logic.
    auto result = new BinaryExpression
        (*operatorToken, leftExpression, rightExpression);
    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeUnaryExpression, Expression *,
     Token *, operatorToken, Expression *, expression)
{
    Expression *result = 0;

    // Attempt to fold constant expression if applicable.
    // Note that not all valid constant expressions may folded (i.e. leaving
    // the operation to run-time). In such cases, the folding routine returns
    // a null pointer.
    auto *constantExpression = dynamic_cast<ConstantExpression *>(expression);
    if (constantExpression)
    {
        try
        {
            result = constantExpression->FoldUnary(operatorToken->type);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
        if (result)
            (SourceElement &)*result = *operatorToken;
        delete expression;
    }

    if (result == 0)
        result = new UnaryExpression(*operatorToken, expression);

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeCallExpression, Expression *,
     Expression *, functionExpression, ArgumentList *, argumentList)
{
    // Ensure named arguments follow positional ones.
    bool positionalArgumentAllowed = true;
    unsigned position = 1;
    for (auto iter = argumentList->ArgumentsBegin();
         iter != argumentList->ArgumentsEnd(); iter++, position++)
    {
        auto argument = *iter;

        if (argument->HasName())
            positionalArgumentAllowed = false;
        else if (!positionalArgumentAllowed)
        {
            ostringstream oss;
            oss
                << "Unnamed argument " << position
                << " follows named argument(s)";
            ReportError(oss.str());
        }
    }

    return new CallExpression(functionExpression, argumentList);
}

DEFINE_ACTION
    (MakeSlicingExpression, Expression *,
     Expression *, sequenceExpression, RangeExpression *, sliceExpression)
{
    return new ElementExpression(sequenceExpression, sliceExpression);
}

DEFINE_ACTION
    (MakeElementExpression, Expression *,
     Expression *, sequenceExpression, Expression *, indexExpression)
{
    return new ElementExpression(sequenceExpression, indexExpression);
}

DEFINE_ACTION
    (MakeMemberExpression, Expression *,
     Expression *, expression, Token *, nameToken)
{
    auto result = new MemberExpression(expression, *nameToken);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeVariableExpression, Expression *,
     Token *, nameToken)
{
    auto result = new VariableExpression(*nameToken);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeLiteralExpression, Expression *,
     ConstantExpression *, expression)
{
    return expression;
}

DEFINE_ACTION
    (MakeDictionaryExpression, Expression *,
     DictionaryExpression *, dictionaryExpression)
{
    return dictionaryExpression;
}

DEFINE_ACTION
    (MakeSetExpression, Expression *,
     SetExpression *, setExpression)
{
    return setExpression;
}

DEFINE_ACTION
    (MakeListExpression, Expression *,
     ListExpression *, listExpression)
{
    return listExpression;
}

DEFINE_ACTION
    (MakeRangeExpression, Expression *,
     RangeExpression *, rangeExpression)
{
    return rangeExpression;
}

DEFINE_ACTION
    (MakeTupleExpression, Expression *,
     Token *, token,
     Expression *, leftExpression, Expression *, rightExpression)
{
    auto result = dynamic_cast<TupleExpression *>(leftExpression);
    if (result == 0)
    {
        result = new TupleExpression(*token);
        if (leftExpression)
            result->Add(leftExpression);
    }
    if (rightExpression)
        result->Add(rightExpression);
    delete token;
    return result;
}

DEFINE_ACTION
    (MakeConstantExpression, ConstantExpression *,
     Token *, token)
{
    auto result = new ConstantExpression(*token);
    delete token;
    return result;
}

DEFINE_ACTION
    (AssignExpression, Expression *, Expression *, expression)
{
    return expression;
}

DEFINE_ACTION
    (MakeEmptyImportNameList, ImportNameList *, int, _)
{
    return new ImportNameList;
}

DEFINE_ACTION
    (AddImportNameToList, ImportNameList *,
     ImportNameList *, importNameList, ImportName *, importName)
{
    importNameList->Add(importName);
    return importNameList;
}

DEFINE_ACTION
    (MakeImportName, ImportName *,
     Token *, importNameToken, Token *, importNameAsToken)
{
    auto result = importNameAsToken ?
        new ImportName(*importNameToken, *importNameAsToken) :
        new ImportName(*importNameToken);
    delete importNameToken;
    delete importNameAsToken;
    return result;
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
     Token *, nameToken, Expression *, defaultExpression)
{
    auto result = new Parameter(*nameToken, defaultExpression);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeEmptyArgumentList, ArgumentList *, int, _)
{
    return new ArgumentList;
}

DEFINE_ACTION
    (AddArgumentToList, ArgumentList *,
     ArgumentList *, argumentList, Argument *, argument)
{
    argumentList->Add(argument);
    return argumentList;
}

DEFINE_ACTION
    (MakeArgument, Argument *,
     Token *, nameToken, Expression *, valueExpression)
{
    auto result = nameToken != 0 ?
        new Argument(*nameToken, valueExpression) :
        new Argument(valueExpression);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeEmptyVariableList, VariableList *, int, _)
{
    return new VariableList;
}

DEFINE_ACTION
    (AddVariableToList, VariableList *,
     VariableList *, variableList, Token *, nameToken)
{
    variableList->Add(*nameToken);
    delete nameToken;
    return variableList;
}

DEFINE_ACTION
    (AssignVariableList, VariableList *,
     VariableList *, variableList)
{
    return variableList;
}

DEFINE_ACTION
    (MakeEmptyDictionary, DictionaryExpression *, Token *, token)
{
    auto result = token != 0 ?
        new DictionaryExpression(*token) : new DictionaryExpression;
    delete token;
    return result;
}

DEFINE_ACTION
    (AddEntryToDictionary, DictionaryExpression *,
     DictionaryExpression *, dictionaryExpression,
     KeyValuePair *, keyValuePair)
{
    dictionaryExpression->Add(keyValuePair);
    return dictionaryExpression;
}

DEFINE_ACTION
    (AssignDictionary, DictionaryExpression *,
     DictionaryExpression *, dictionaryExpression)
{
    return dictionaryExpression;
}

DEFINE_ACTION
    (MakeEmptySet, SetExpression *, Token *, token)
{
    auto result = new SetExpression;
    if (token != 0)
        (SourceElement &)*result = *token;
    delete token;
    return result;
}

DEFINE_ACTION
    (AssignSet, SetExpression *,
     Token *, token, ListExpression *, listExpression)
{
    auto result = new SetExpression;
    if (token != 0)
        (SourceElement &)*result = *token;
    delete token;
    for (auto iter = listExpression->ExpressionsBegin();
         iter != listExpression->ExpressionsEnd(); iter++)
    {
        auto expression = *iter;
        result->Add(expression);
    }
    return result;
}

DEFINE_ACTION
    (MakeEmptyList, ListExpression *, Token *, token)
{
    auto result = new ListExpression;
    if (token != 0)
        (SourceElement &)*result = *token;
    delete token;
    return result;
}

DEFINE_ACTION
    (AddExpressionToList, ListExpression *,
     ListExpression *, listExpression, Expression *, expression)
{
    listExpression->Add(expression);
    return listExpression;
}

DEFINE_ACTION
    (AssignList, ListExpression *,
     Token *, token, ListExpression *, listExpression)
{
    if (listExpression->Empty())
        (SourceElement &)*listExpression = *token;
    delete token;
    return listExpression;
}

DEFINE_ACTION
    (MakeKeyValuePair, KeyValuePair *,
     Expression *, keyExpression, Expression *, valueExpression)
{
    return new KeyValuePair(keyExpression, valueExpression);
}

DEFINE_ACTION
    (MakeRange, RangeExpression *,
     Token *, token,
     Expression *, startExpression, Expression *, endExpression,
     Expression *, stepExpression)
{
    auto result = new RangeExpression
        (*token, startExpression, endExpression, stepExpression);
    delete token;
    return result;
}

DEFINE_ACTION
    (AssignToken, Token *, Token *, token)
{
    return token;
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

void Compiler::ReportError(const string &error)
{
    errorStream
        << currentSourceLocation.line << ':'
        << currentSourceLocation.column << ": Error: "
        << error << endl;
    errorCount++;
}
