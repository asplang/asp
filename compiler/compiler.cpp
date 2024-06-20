//
// Asp compiler implementation.
//

#include "compiler.h"
#include "asp.h"
#include "grammar.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "function.hpp"
#include "instruction.hpp"
#include "symbols.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdint>

using namespace std;

static const string ModuleSuffix = ".asp";
static const SourceLocation NoSourceLocation;

Compiler::Compiler
    (ostream &errorStream,
     SymbolTable &symbolTable, Executable &executable) :
    errorStream(errorStream),
    symbolTable(symbolTable),
    executable(executable),
    topLocation(executable.Insert(new NullInstruction, NoSourceLocation))
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

    // Read and check application spec version.
    uint8_t version;
    specStream >> version;
    if (version > 0x01)
    {
        ostringstream oss;
        oss
            << "Unrecognized application specification file version: "
            << static_cast<unsigned>(version);
        throw oss.str();
    }

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
        symbolTable.Symbol(name);
    }
}

void Compiler::AddModule(const string &moduleName)
{
    // Ignore import of the system module. It is implicitly imported into
    // every module anyway.
    if (moduleName == AspSystemModuleName)
        return;

    // Store the top-level module name.
    if (moduleNames.empty())
        topModuleName = moduleName;

    // Add the module only if it has never been added before.
    auto iter = moduleNames.find(moduleName);
    if (iter == moduleNames.end())
    {
        symbolTable.Symbol(moduleName);
        moduleNames.insert(moduleName);
        moduleNamesToImport.push_back(moduleName);
    }
}

void Compiler::AddModuleFileName(const string &moduleFileName)
{
    // Ensure the file name includes the proper suffix.
    bool bad = moduleFileName.size() <= ModuleSuffix.size();
    auto suffixIndex = moduleFileName.size() - ModuleSuffix.size();
    bad = bad || moduleFileName.substr(suffixIndex) != ModuleSuffix;
    if (bad)
    {
        ostringstream oss;
        oss
            << "Module file name '" << moduleFileName
            << "' does not end with '" << ModuleSuffix << '\'';
        ReportError(oss.str());
    }

    // Strip off the suffix and add the module name.
    string moduleName = moduleFileName.substr(0, suffixIndex);
    if (moduleName == AspSystemModuleName)
    {
        ostringstream oss;
        oss
            << "Cannot use module name '" << moduleName
            << "' which is reserved for system use";
        ReportError(oss.str());
    }

    AddModule(moduleName);
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
    executable.Insert
        (new LoadModuleInstruction
            (symbolTable.Symbol(topModuleName), "Load top-level module"),
         NoSourceLocation);
    executable.Insert
        (new EndInstruction("End script"),
         NoSourceLocation);
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
    if (result && result->sourceLocation.line != 0) \
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
        auto moduleLocation = executable.Insert
            (new NullInstruction, NoSourceLocation);
        executable.MarkModuleLocation(currentModuleName, moduleLocation);

        executable.PushLocation(topLocation);
        {
            ostringstream oss;
            oss << "Add address of module " << currentModuleName;
            executable.Insert
                (new AddModuleInstruction
                    (currentModuleSymbol, moduleLocation, oss.str()),
                 NoSourceLocation);
        }
        executable.PopLocation();

        currentSourceLocation = NoSourceLocation;
        module->Emit(executable);

        const SourceElement *finalSourceElement = module->FinalStatement();
        if (finalSourceElement == nullptr)
            finalSourceElement = module;
        executable.Insert
            (new ExitModuleInstruction("Exit module"),
             finalSourceElement->sourceLocation);
    }
    catch (const pair<SourceElement, string> &e)
    {
        ReportError(e.second, e.first);
    }
    catch (const string &e)
    {
        ReportError(e);
    }

    delete module;

    return nullptr;
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

DEFINE_ACTION
    (MakeAssertStatement, Statement *,
     Expression *, expression)
{
    return new AssertStatement(expression);
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
     TargetExpression *, targetExpression, Expression *,expression,
     Block *, trueBlock, Block *, falseBlock)
{
    return new ForStatement
        (targetExpression, expression, trueBlock, falseBlock);
}

DEFINE_ACTION
    (MakeDefStatement, Statement *,
     Token *, nameToken, ParameterList *, parameterList, Block *, block)
{
    if (!parameterList->HasSourceLocation())
        (SourceElement &)*parameterList = *nameToken;

    // Ensure the validity of the order of parameter types.
    ValidFunctionDefinition validFunctionDefinition;
    for (auto iter = parameterList->ParametersBegin();
         validFunctionDefinition.IsValid() &&
         iter != parameterList->ParametersEnd();
         iter++)
    {
        const auto &parameter = **iter;

        string error = validFunctionDefinition.AddParameter
            (parameter.Name(), parameter.GetType(), parameter.HasDefault());
        if (!error.empty())
            ReportError(error, parameter);
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
    (MakeConditionalExpression, Expression *,
     Token *, operatorToken, Expression *, conditionExpression,
     Expression *, trueExpression, Expression *, falseExpression)
{
    // Attempt to fold constant expression.
    Expression *result = nullptr;
    try
    {
        result = FoldTernaryExpression
            (operatorToken->type,
             conditionExpression, trueExpression, falseExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }
    if (result)
    {
        (SourceElement &)*result = *operatorToken;
        if (conditionExpression != result)
            delete conditionExpression;
        if (trueExpression != result)
            delete trueExpression;
        if (falseExpression != result)
            delete falseExpression;
    }
    else
        result = new ConditionalExpression
            (*operatorToken, conditionExpression,
             trueExpression, falseExpression);

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeShortCircuitLogicalExpression, Expression *,
     Token *, operatorToken,
     Expression *, leftExpression, Expression *, rightExpression)
{
    // Attempt to fold constant expression.
    Expression *result = nullptr;
    try
    {
        result = FoldBinaryExpression
            (operatorToken->type, leftExpression, rightExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }
    if (result)
    {
        (SourceElement &)*result = *operatorToken;
        if (leftExpression != result)
            delete leftExpression;
        if (rightExpression != result)
            delete rightExpression;
    }
    else
    {
        auto castLeftExpression = dynamic_cast<ShortCircuitLogicalExpression *>
            (leftExpression);
        if (castLeftExpression != nullptr &&
            castLeftExpression->OperatorTokenType() == operatorToken->type)
        {
            castLeftExpression->Add(rightExpression);
            result = leftExpression;
        }
        else
        {
            result = new ShortCircuitLogicalExpression
                (*operatorToken, leftExpression, rightExpression);
        }
    }

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeBinaryExpression, Expression *,
     Token *, operatorToken,
     Expression *, leftExpression, Expression *, rightExpression)
{
    // Attempt to fold constant expression.
    Expression *result = nullptr;
    try
    {
        result = FoldBinaryExpression
            (operatorToken->type, leftExpression, rightExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }
    if (result)
    {
        (SourceElement &)*result = *operatorToken;
        if (leftExpression != result)
            delete leftExpression;
        if (rightExpression != result)
            delete rightExpression;
    }
    else
        result = new BinaryExpression
            (*operatorToken, leftExpression, rightExpression);

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeUnaryExpression, Expression *,
     Token *, operatorToken, Expression *, expression)
{
    // Attempt to fold constant expression.
    Expression *result = nullptr;
    try
    {
        result = FoldUnaryExpression(operatorToken->type, expression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }
    if (result)
    {
        (SourceElement &)*result = *operatorToken;
        if (expression != result)
            delete expression;
    }
    else
        result = new UnaryExpression(*operatorToken, expression);

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeCallExpression, Expression *,
     Expression *, functionExpression, ArgumentList *, argumentList)
{
    if (!argumentList->HasSourceLocation())
        (SourceElement &)*argumentList = *functionExpression;

    // Ensure named arguments follow positional ones.
    bool namedSeen = false, dictionaryGroupSeen = false;
    unsigned position = 1;
    for (auto iter = argumentList->ArgumentsBegin();
         iter != argumentList->ArgumentsEnd(); iter++, position++)
    {
        auto &argument = **iter;
        Argument::Type type = argument.GetType();
        bool isPositional =
            type == Argument::Type::NonGroup && !argument.HasName();

        if (isPositional && (namedSeen || dictionaryGroupSeen))
        {
            ostringstream oss;
            oss
                << "Positional argument " << position << " follows "
                << (dictionaryGroupSeen ? "dictionary group" : "named")
                << " argument";
            ReportError(oss.str(), argument);
        }
        else if (type == Argument::Type::IterableGroup && dictionaryGroupSeen)
        {
            ostringstream oss;
            oss
                << "Iterable argument " << position
                << " follows dictionary group argument";
            ReportError(oss.str(), argument);
        }

        if (type == Argument::Type::DictionaryGroup)
            dictionaryGroupSeen = true;
        else if (argument.HasName())
            namedSeen = true;
    }

    return new CallExpression(functionExpression, argumentList);
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
    (MakeSymbolExpression, Expression *,
     Token *, operatorToken, Token *, nameToken)
{
    auto result = new SymbolExpression(*operatorToken, *nameToken);
    delete operatorToken;
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
    if (leftExpression == nullptr || result == nullptr || result->IsEnclosed())
    {
        result = new TupleExpression(*token);
        if (leftExpression)
            result->Add(leftExpression);
        else
            result->Enclose();
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
    (MakeJuxtaposeConstantExpression, ConstantExpression *,
     ConstantExpression *, leftExpression,
     ConstantExpression *, rightExpression)
{
    // Only strings can be juxtaposed.
    if (leftExpression->GetType() != ConstantExpression::Type::String ||
        rightExpression->GetType() != ConstantExpression::Type::String)
        throw string("Syntax error");

    // Fold the juxtaposed expression using addition.
    Expression *resultExpression = FoldBinaryExpression
        (TOKEN_PLUS, leftExpression, rightExpression);
    if (resultExpression == nullptr)
        throw string("Internal error: Error folding juxtaposed strings");
    if (leftExpression != resultExpression)
        delete leftExpression;
    if (rightExpression != resultExpression)
        delete rightExpression;
    auto result = dynamic_cast<ConstantExpression *>(resultExpression);
    if (result == nullptr)
        throw string("Internal error: Error folding juxtaposed strings");

    return result;
}

DEFINE_ACTION
    (MakeEnclosedExpression, Expression *, Expression *, expression)
{
    // Enclose the expression in parentheses. This prevents tuples in
    // parentheses from being added to.
    expression->Enclose();
    return expression;
}

DEFINE_ACTION
    (AssignExpression, Expression *, Expression *, expression)
{
    return expression;
}

DEFINE_ACTION
    (AssignConstantExpression, ConstantExpression *,
     ConstantExpression *, constantExpression)
{
    return constantExpression;
}

DEFINE_ACTION
    (MakeTargetExpression, TargetExpression *,
     Token *, token,
     TargetExpression *, leftTargetExpression,
     TargetExpression *, rightTargetExpression)
{
    TargetExpression *result = leftTargetExpression;
    if (leftTargetExpression == nullptr ||
        !leftTargetExpression->IsTuple() ||
        leftTargetExpression->IsEnclosed())
    {
        result = new TargetExpression(*token);
        if (leftTargetExpression != nullptr)
            result->Add(leftTargetExpression);
        else if (result->IsTuple())
            result->Enclose();
    }

    if (rightTargetExpression != nullptr)
        result->Add(rightTargetExpression);

    delete token;
    return result;
}

DEFINE_ACTION
    (MakeEnclosedTargetExpression, TargetExpression *,
     TargetExpression *, targetExpression)
{
    // Enclose the expression in parentheses. This prevents target expressions
    // in parentheses from being added to.
    targetExpression->Enclose();
    return targetExpression;
}

DEFINE_ACTION
    (AssignTargetExpression, TargetExpression *,
     TargetExpression *, targetExpression)
{
    return targetExpression;
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
    auto result = new Parameter
        (*nameToken, Parameter::Type::Positional, defaultExpression);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeTupleGroupParameter, Parameter *,
     Token *, nameToken)
{
    auto result = new Parameter
        (*nameToken, Parameter::Type::TupleGroup);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDictionaryGroupParameter, Parameter *,
     Token *, nameToken)
{
    auto result = new Parameter
        (*nameToken, Parameter::Type::DictionaryGroup);
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
    auto result = nameToken != nullptr ?
        new Argument(*nameToken, valueExpression) :
        new Argument(valueExpression);
    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeIterableGroupArgument, Argument *,
     Expression *, valueExpression)
{
    auto constantExpression = dynamic_cast<ConstantExpression *>
        (valueExpression);
    if (constantExpression != nullptr && !constantExpression->IsString())
        ReportError("Invalid type for iterable group argument");

    return new Argument(valueExpression, Argument::Type::IterableGroup);
}

DEFINE_ACTION
    (MakeDictionaryGroupArgument, Argument *,
     Expression *, valueExpression)
{
    auto constantExpression = dynamic_cast<ConstantExpression *>
        (valueExpression);
    if (constantExpression != nullptr)
        ReportError("Invalid type for dictionary group argument");

    return new Argument(valueExpression, Argument::Type::DictionaryGroup);
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
    auto result = token != nullptr ?
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
    auto result = new SetExpression(*token);
    delete token;
    return result;
}

DEFINE_ACTION
    (AssignSet, SetExpression *,
     Token *, token, ListExpression *, listExpression)
{
    auto result = new SetExpression(*token);
    delete token;
    while (auto expression = listExpression->PopFront())
        result->Add(expression);
    delete listExpression;
    return result;
}

DEFINE_ACTION
    (MakeEmptyList, ListExpression *, Token *, token)
{
    if (token != nullptr)
    {
        auto result = new ListExpression(*token);
        delete token;
        return result;
    }
    else
        return new ListExpression;
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
    if (listExpression->IsEmpty())
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
    ReportError(error, currentSourceLocation);
}

void Compiler::ReportError
    (const string &error, const SourceElement &sourceElement)
{
    ReportError(error, sourceElement.sourceLocation);
}

void Compiler::ReportError
    (const string &error, const SourceLocation &sourceLocation)
{
    if (sourceLocation.Defined())
        errorStream
            << sourceLocation.fileName << ':'
            << sourceLocation.line << ':'
            << sourceLocation.column << ": ";
    errorStream << "Error: " << error << endl;
    errorCount++;
}
