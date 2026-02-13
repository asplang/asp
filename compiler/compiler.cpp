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
#include <string>
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
    appModuleNames.insert(AspSystemModuleName);
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
    if (version > 2u)
    {
        ostringstream oss;
        oss
            << "Unrecognized application specification file version: "
            << static_cast<unsigned>(version);
        throw oss.str();
    }

    // Read the application specification check value and store it.
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
    char delim = version >= 2u ? ' ' : '\n';
    bool storeAppModuleNames = version >= 2u;
    while (true)
    {
        string name;
        getline(specStream, name, delim);
        if (specStream.eof())
            break;
        if (name.empty())
        {
            storeAppModuleNames = false;
            continue;
        }
        if (storeAppModuleNames)
            appModuleNames.insert(name);
        symbolTable.Symbol(name);
    }
}

void Compiler::AddModule(const string &moduleName)
{
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
        return;
    }

    // Strip off the suffix to obtain the module name.
    string moduleName = moduleFileName.substr(0, suffixIndex);

    // Ensure the module name is not already defined by the application.
    if (IsAppModule(moduleName))
    {
        ostringstream oss;
        oss
            << "Cannot use module name '" << moduleName
            << "' which is reserved ";
        if (moduleName == AspSystemModuleName)
            oss << "for system use";
        else
            oss << "as an application module";
        ReportError(oss.str());
        return;
    }

    // Add the module.
    AddModule(moduleName);
}

pair<string, list<SourceElement> > Compiler::NextModule()
{
    if (moduleNamesToImport.empty())
        currentModuleName.clear();
    else
    {
        // Switch to the next module.
        currentModuleName = moduleNamesToImport.front();
        currentModuleSymbol = symbolTable.Symbol(currentModuleName);
        moduleNamesToImport.pop_front();
    }

    // Gather all the import source locations that reference the module.
    list<SourceElement> sourceReferences;
    auto importedModuleIter = importedModules.find(currentModuleName);
    if (importedModuleIter != importedModules.end())
    {
        for (const auto &sourceElement: importedModuleIter->second)
            sourceReferences.emplace_back(sourceElement);
    }

    return make_pair(currentModuleName, sourceReferences);
}

bool Compiler::IsAppModule(const string &moduleName) const
{
    return appModuleNames.find(moduleName) != appModuleNames.end();
}

void Compiler::SetSourceLocation(const SourceLocation &sourceLocation)
{
    currentSourceLocation = sourceLocation;
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
    if (statement != nullptr)
    {
        try
        {
            auto blockStatement = dynamic_cast<BlockStatement *>(statement);
            if (blockStatement == nullptr)
                block->Add(statement);
            else
            {
                blockStatement->MoveStatements(block);
                delete statement;
            }
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

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
    AssignmentStatement *result = nullptr;

    if (targetExpression != nullptr && valueAssignmentStatement != nullptr)
    {
        try
        {
            result = new AssignmentStatement
                (*assignmentToken, targetExpression, valueAssignmentStatement);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete assignmentToken;
    return result;
}

DEFINE_ACTION
    (MakeSingleAssignmentStatement, AssignmentStatement *,
     Token *, assignmentToken, Expression *, targetExpression,
     Expression *, valueExpression)
{
    AssignmentStatement *result = nullptr;

    if (targetExpression != nullptr && valueExpression != nullptr)
    {
        try
        {
            result = new AssignmentStatement
                (*assignmentToken, targetExpression, valueExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

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
    InsertionStatement *result = nullptr;

    if (insertionStatement != nullptr && itemExpression != nullptr)
    {
        try
        {
            result = new InsertionStatement
                (*insertionToken, insertionStatement, itemExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeMultiplePairInsertionStatement, InsertionStatement *,
     Token *, insertionToken, InsertionStatement *, insertionStatement,
     KeyValuePair *, keyValuePair)
{
    InsertionStatement *result = nullptr;

    if (insertionStatement != nullptr && keyValuePair != nullptr)
    {
        try
        {
            result = new InsertionStatement
                (*insertionToken, insertionStatement, keyValuePair);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeSingleValueInsertionStatement, InsertionStatement *,
     Token *, insertionToken, Expression *, containerExpression,
     Expression *, itemExpression)
{
    InsertionStatement *result = nullptr;

    if (containerExpression != nullptr && itemExpression != nullptr)
    {
        try
        {
            result = new InsertionStatement
                (*insertionToken, containerExpression, itemExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete insertionToken;
    return result;
}

DEFINE_ACTION
    (MakeSinglePairInsertionStatement, InsertionStatement *,
     Token *, insertionToken, Expression *, containerExpression,
     KeyValuePair *, keyValuePair)
{
    InsertionStatement *result = nullptr;

    if (containerExpression != nullptr && keyValuePair != nullptr)
    {
        try
        {
            result = new InsertionStatement
                (*insertionToken, containerExpression, keyValuePair);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

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
    if (expression != nullptr)
    {
        try
        {
            return new ExpressionStatement(expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeImportStatement, Statement *,
     ImportNameList *, moduleNameList)
{
    if (moduleNameList != nullptr)
    {
        try
        {
            for (auto iter = moduleNameList->NamesBegin();
                 iter != moduleNameList->NamesEnd(); iter++)
            {
                const auto &importName = *iter;

                AddModule(importName->Name());

                auto importedModuleIter = importedModules.emplace
                    (importName->Name(), list<SourceElement>()).first;
                importedModuleIter->second.push_back(*moduleNameList);
            }

            return new ImportStatement(moduleNameList);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeFromImportStatement, Statement *,
     ImportName *, moduleName, ImportNameList *, memberNameList)
{
    if (moduleName != nullptr && memberNameList != nullptr)
    {
        try
        {
            AddModule(moduleName->Name());

            auto importedModuleIter = importedModules.emplace
                (moduleName->Name(), list<SourceElement>()).first;
            importedModuleIter->second.push_back(*moduleName);

            auto moduleNameList = new ImportNameList;
            moduleNameList->Add(moduleName);
            return new ImportStatement(moduleNameList, memberNameList);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeGlobalStatement, Statement *,
     VariableList *, variableList)
{
    if (variableList != nullptr)
    {
        try
        {
            return new GlobalStatement(variableList);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeLocalStatement, Statement *,
     VariableList *, variableList)
{
    if (variableList != nullptr)
    {
        try
        {
            return new LocalStatement(variableList);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeDelStatement, Statement *,
     Expression *, expression)
{
    if (expression != nullptr)
    {
        try
        {
            return new DelStatement(expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeReturnStatement, Statement *,
     Token *, keywordToken, Expression *, expression)
{
    Statement *result = nullptr;

    // Note that the expression may be unspecified (i.e., null).
    try
    {
        result = new ReturnStatement(*keywordToken, expression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete keywordToken;
    return result;
}

DEFINE_ACTION
    (MakeAssertStatement, Statement *,
     Expression *, expression)
{
    if (expression != nullptr)
    {
        try
        {
            return new AssertStatement(expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION(MakeBreakStatement, Statement *, Token *, token)
{
    Statement *result = nullptr;

    try
    {
        result = new BreakStatement(*token);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION(MakeContinueStatement, Statement *, Token *, token)
{
    Statement *result = nullptr;

    try
    {
        result = new ContinueStatement(*token);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION(MakePassStatement, Statement *, Token *, token)
{
    Statement *result = nullptr;

    try
    {
        result = new PassStatement(*token);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (MakeIfElseIfStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock, IfStatement *, ifStatement)
{
    if (expression != nullptr && trueBlock != nullptr &&
        ifStatement != nullptr)
    {
        try
        {
            return new IfStatement(expression, trueBlock, ifStatement);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeIfElseStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock, Block *, falseBlock)
{
    if (expression != nullptr &&
        trueBlock != nullptr && falseBlock != nullptr)
    {
        try
        {
            return new IfStatement(expression, trueBlock, falseBlock);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeIfStatement, IfStatement *,
     Expression *, expression, Block *, trueBlock)
{
    if (expression != nullptr && trueBlock != nullptr)
    {
        try
        {
            return new IfStatement(expression, trueBlock);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeWhileStatement, Statement *,
     Expression *, expression, Block *, trueBlock, Block *, falseBlock)
{
    // Note that the false expression may be unspecified (i.e., null).
    if (expression != nullptr && trueBlock != nullptr)
    {
        try
        {
            return new WhileStatement(expression, trueBlock, falseBlock);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeForStatement, Statement *,
     Expression *, targetExpression, Expression *,expression,
     Block *, trueBlock, Block *, falseBlock)
{
    // Note that the target or false expressions may be unspecified
    // (i.e., null).
    if (expression != nullptr && trueBlock != nullptr)
    {
        try
        {
            return new ForStatement
                (targetExpression, expression, trueBlock, falseBlock);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeDefStatement, Statement *,
     Token *, nameToken, ParameterList *, parameterList, Block *, block)
{
    Statement *result = nullptr;

    if (parameterList != nullptr && block != nullptr)
    {
        try
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
                    (parameter.Name(), parameter.GetType(),
                     parameter.HasDefault());
                if (!error.empty())
                    ReportError(error, parameter);
            }

            result = new DefStatement(*nameToken, parameterList, block);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeBlockStatement, Statement *, Block *, block)
{
    if (block != nullptr)
    {
        try
        {
            return new BlockStatement(block);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (AssignStatement, Statement *, Statement *, statement)
{
    return statement;
}

DEFINE_ACTION
    (MakeAssignmentExpression, Expression *,
     Token *, operatorToken,
     Expression *, targetExpression, Expression *, valueExpression)
{
    Expression *result = nullptr;

    if (targetExpression != nullptr && valueExpression != nullptr)
    {
        try
        {
            result = new AssignmentExpression
                (*operatorToken, targetExpression, valueExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeConditionalExpression, Expression *,
     Token *, operatorToken, Expression *, conditionExpression,
     Expression *, trueExpression, Expression *, falseExpression)
{
    Expression *result = nullptr;

    if (conditionExpression != nullptr &&
        trueExpression != nullptr && falseExpression != nullptr)
    {
        try
        {
            // Attempt to fold constant expression.
            result = FoldTernaryExpression
                (operatorToken->type,
                 conditionExpression, trueExpression, falseExpression);
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
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeShortCircuitLogicalExpression, Expression *,
     Token *, operatorToken,
     Expression *, leftExpression, Expression *, rightExpression)
{
    Expression *result = nullptr;

    if (leftExpression != nullptr && rightExpression != nullptr)
    {
        try
        {
            // Attempt to fold constant expression.
            result = FoldBinaryExpression
                (operatorToken->type, leftExpression, rightExpression);
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
                auto castLeftExpression =
                    dynamic_cast<ShortCircuitLogicalExpression *>
                    (leftExpression);
                if (castLeftExpression != nullptr &&
                    castLeftExpression->OperatorTokenType()
                    == operatorToken->type)
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
        }
        catch (const string &error)
        {
            ReportError(error);
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
    Expression *result = nullptr;

    if (leftExpression != nullptr && rightExpression != nullptr)
    {
        try
        {
            // Attempt to fold constant expression.
            result = FoldBinaryExpression
                (operatorToken->type, leftExpression, rightExpression);
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
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeUnaryExpression, Expression *,
     Token *, operatorToken, Expression *, expression)
{
    Expression *result = nullptr;

    if (expression != nullptr)
    {
        // Attempt to fold constant expression.
        try
        {
            result = FoldUnaryExpression(operatorToken->type, expression);
            if (result)
            {
                (SourceElement &)*result = *operatorToken;
                if (expression != result)
                    delete expression;
            }
            else
                result = new UnaryExpression(*operatorToken, expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete operatorToken;
    return result;
}

DEFINE_ACTION
    (MakeCallExpression, Expression *,
     Expression *, functionExpression, ArgumentList *, argumentList)
{
    if (functionExpression != nullptr && argumentList != nullptr)
    {
        try
        {
            if (!argumentList->HasSourceLocation())
                (SourceElement &)*argumentList = *functionExpression;

            // Ensure named arguments follow positional ones.
            bool namedSeen = false, dictionaryGroupSeen = false;
            unsigned position = 1;
            for (auto iter = argumentList->ArgumentsBegin();
                 iter != argumentList->ArgumentsEnd(); iter++, position++)
            {
                const auto &argument = **iter;
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
                else if (type == Argument::Type::IterableGroup &&
                         dictionaryGroupSeen)
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
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeElementExpression, Expression *,
     Expression *, sequenceExpression, Expression *, indexExpression)
{
    if (sequenceExpression != nullptr && indexExpression != nullptr)
    {
        try
        {
            return new ElementExpression(sequenceExpression, indexExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeMemberExpression, Expression *,
     Expression *, expression, Token *, nameToken)
{
    Expression *result = nullptr;

    if (expression != nullptr)
    {
        try
        {
            result = new MemberExpression(expression, *nameToken);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeVariableExpression, Expression *,
     Token *, nameToken)
{
    Expression *result = nullptr;

    try
    {
        result = new VariableExpression(*nameToken);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeSymbolExpression, Expression *,
     Token *, operatorToken, Token *, nameToken)
{
    Expression *result = nullptr;

    try
    {
        result = new SymbolExpression(*operatorToken, *nameToken);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

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
    TupleExpression *result = nullptr;

    // Note that either expression may be unspecified (i.e., null).
    try
    {
        result = dynamic_cast<TupleExpression *>(leftExpression);
        if (leftExpression == nullptr ||
            result == nullptr || result->IsEnclosed())
        {
            result = new TupleExpression(*token);
            if (leftExpression)
                result->Add(leftExpression);
            else
                result->Enclose();
        }

        if (rightExpression != nullptr)
            result->Add(rightExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (MakeConstantExpression, ConstantExpression *,
     Token *, token)
{
    ConstantExpression *result = nullptr;

    try
    {
        result = new ConstantExpression(*token);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (MakeJuxtaposeConstantExpression, ConstantExpression *,
     ConstantExpression *, leftExpression,
     ConstantExpression *, rightExpression)
{
    ConstantExpression *result = nullptr;

    if (leftExpression != nullptr && rightExpression != nullptr)
    {
        try
        {
            // Only strings can be juxtaposed.
            if (leftExpression->GetType()
                != ConstantExpression::Type::String ||
                rightExpression->GetType()
                != ConstantExpression::Type::String)
                throw string("Syntax error");

            // Fold the juxtaposed expressions using addition.
            Expression *resultExpression = FoldBinaryExpression
                (TOKEN_PLUS, leftExpression, rightExpression);
            if (resultExpression == nullptr)
                throw string
                    ("Internal error: Error folding juxtaposed strings");
            if (leftExpression != resultExpression)
                delete leftExpression;
            if (rightExpression != resultExpression)
                delete rightExpression;
            result = dynamic_cast<ConstantExpression *>(resultExpression);
            if (result == nullptr)
                throw string
                    ("Internal error: Error folding juxtaposed strings");
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return result;
}

DEFINE_ACTION
    (MakeEnclosedExpression, Expression *, Expression *, expression)
{
    if (expression != nullptr)
    {
        try
        {
            // Enclose the expression in parentheses. This prevents tuples in
            // parentheses from being added to.
            expression->Enclose();
            return expression;
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
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
    (MakeEmptyImportNameList, ImportNameList *, int, _)
{
    return new ImportNameList;
}

DEFINE_ACTION
    (AddImportNameToList, ImportNameList *,
     ImportNameList *, importNameList, ImportName *, importName)
{
    if (importNameList != nullptr && importName != nullptr)
    {
        try
        {
            importNameList->Add(importName);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return importNameList;
}

DEFINE_ACTION
    (MakeImportName, ImportName *,
     Token *, importNameToken, Token *, importNameAsToken)
{
    ImportName *result = nullptr;

    // Note that the "as" name may be unspecified (i.e., null).
    try
    {
        result = importNameAsToken ?
            new ImportName(*importNameToken, *importNameAsToken) :
            new ImportName(*importNameToken);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

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
    if (parameterList != nullptr && parameter != nullptr)
    {
        try
        {
            parameterList->Add(parameter);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return parameterList;
}

DEFINE_ACTION
    (MakeParameter, Parameter *,
     Token *, nameToken, Expression *, defaultExpression)
{
    Parameter *result = nullptr;

    // Note that the default expression may be unspecified (i.e., null).
    try
    {
        result = new Parameter
            (*nameToken, Parameter::Type::Positional, defaultExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeTupleGroupParameter, Parameter *,
     Token *, nameToken)
{
    Parameter *result = nullptr;

    try
    {
        result = new Parameter
            (*nameToken, Parameter::Type::TupleGroup);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeDictionaryGroupParameter, Parameter *,
     Token *, nameToken)
{
    Parameter *result = nullptr;

    try
    {
        result = new Parameter
            (*nameToken, Parameter::Type::DictionaryGroup);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

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
    if (argumentList != nullptr && argument != nullptr)
    {
        try
        {
            argumentList->Add(argument);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return argumentList;
}

DEFINE_ACTION
    (MakeArgument, Argument *,
     Token *, nameToken, Expression *, valueExpression)
{
    Argument *result = nullptr;

    // Note that the name may be unspecified (i.e., null).
    if (valueExpression != nullptr)
    {
        try
        {
            result = nameToken != nullptr ?
                new Argument(*nameToken, valueExpression) :
                new Argument(valueExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete nameToken;
    return result;
}

DEFINE_ACTION
    (MakeIterableGroupArgument, Argument *,
     Expression *, valueExpression)
{
    if (valueExpression != nullptr)
    {
        try
        {
            auto constantExpression = dynamic_cast<ConstantExpression *>
                (valueExpression);
            if (constantExpression != nullptr &&
                !constantExpression->IsString())
                ReportError("Invalid type for iterable group argument");

            return new Argument
                (valueExpression, Argument::Type::IterableGroup);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeDictionaryGroupArgument, Argument *,
     Expression *, valueExpression)
{
    if (valueExpression != nullptr)
    {
        try
        {
            auto constantExpression = dynamic_cast<ConstantExpression *>
                (valueExpression);
            if (constantExpression != nullptr)
                ReportError("Invalid type for dictionary group argument");

            return new Argument
                (valueExpression, Argument::Type::DictionaryGroup);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
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
    if (variableList != nullptr)
    {
        try
        {
            variableList->Add(*nameToken);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

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
    DictionaryExpression *result = nullptr;

    try
    {
        result = token != nullptr ?
            new DictionaryExpression(*token) : new DictionaryExpression;
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (AddEntryToDictionary, DictionaryExpression *,
     DictionaryExpression *, dictionaryExpression,
     KeyValuePair *, keyValuePair)
{
    if (dictionaryExpression != nullptr && keyValuePair != nullptr)
    {
        try
        {
            dictionaryExpression->Add(keyValuePair);
            return dictionaryExpression;
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

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
    SetExpression *result = nullptr;

    try
    {
        result = new SetExpression(*token);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (AssignSet, SetExpression *,
     Token *, token, ListExpression *, listExpression)
{
    SetExpression *result = nullptr;

    if (listExpression != nullptr)
    {
        try
        {
            result = new SetExpression(*token);
            while (auto expression = listExpression->PopFront())
                result->Add(expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    delete token;
    delete listExpression;
    return result;
}

DEFINE_ACTION
    (MakeEmptyList, ListExpression *, Token *, token)
{
    ListExpression *result = nullptr;

    try
    {
        if (token != nullptr)
        {
            result = new ListExpression(*token);
        }
        else
            return new ListExpression;
    }
    catch (const string &error)
    {
        ReportError(error);
    }

    delete token;
    return result;
}

DEFINE_ACTION
    (AddExpressionToList, ListExpression *,
     ListExpression *, listExpression, Expression *, expression)
{
    if (listExpression != nullptr && expression != nullptr)
    {
        try
        {
            listExpression->Add(expression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return listExpression;
}

DEFINE_ACTION
    (AssignList, ListExpression *,
     Token *, token, ListExpression *, listExpression)
{
    if (listExpression != nullptr && listExpression->IsEmpty() &&
        token != nullptr)
        (SourceElement &)*listExpression = *token;

    delete token;
    return listExpression;
}

DEFINE_ACTION
    (MakeKeyValuePair, KeyValuePair *,
     Expression *, keyExpression, Expression *, valueExpression)
{
    if (keyExpression != nullptr && valueExpression != nullptr)
    {
        try
        {
            return new KeyValuePair(keyExpression, valueExpression);
        }
        catch (const string &error)
        {
            ReportError(error);
        }
    }

    return nullptr;
}

DEFINE_ACTION
    (MakeRange, RangeExpression *,
     Token *, token,
     Expression *, startExpression, Expression *, endExpression,
     Expression *, stepExpression)
{
    RangeExpression *result = nullptr;

    // Note that any of the expressions may be unspecified (i.e., null).
    try
    {
        result = new RangeExpression
            (*token, startExpression, endExpression, stepExpression);
    }
    catch (const string &error)
    {
        ReportError(error);
    }

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
