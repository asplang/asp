/*
 * Asp compiler definitions.
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"

#ifdef __cplusplus
#include "executable.hpp"
#include "symbol.hpp"
#include <iostream>
#include <deque>
#include <set>
#include <string>
#endif

#ifdef __cplusplus
#define DECLARE_TYPE(T) class T;
#define DECLARE_METHOD(name, ReturnType, ...) \
    friend ReturnType name(Compiler *, __VA_ARGS__); \
    ReturnType name(__VA_ARGS__);
#else
#define DECLARE_TYPE(T) typedef struct T T;
#define DECLARE_METHOD(name, ReturnType, ...) \
    ReturnType name(Compiler *, __VA_ARGS__);
#define ACTION(name, ...) (name(compiler, __VA_ARGS__))
#define FREE_NT(name) (FreeNonTerminal(compiler, name))
#define FREE_TOKEN(name) (FreeToken(compiler, name))
#endif

DECLARE_TYPE(SourceElement)
DECLARE_TYPE(NonTerminal)
DECLARE_TYPE(ConstantExpression)
DECLARE_TYPE(Expression)
DECLARE_TYPE(AssignmentStatement)
DECLARE_TYPE(InsertionStatement)
DECLARE_TYPE(IfStatement)
DECLARE_TYPE(Statement)
DECLARE_TYPE(ImportNameList)
DECLARE_TYPE(ImportName)
DECLARE_TYPE(ParameterList)
DECLARE_TYPE(Parameter)
DECLARE_TYPE(ArgumentList)
DECLARE_TYPE(Argument)
DECLARE_TYPE(DictionaryExpression)
DECLARE_TYPE(SetExpression)
DECLARE_TYPE(ListExpression)
DECLARE_TYPE(KeyValuePair)
DECLARE_TYPE(RangeExpression)
DECLARE_TYPE(TargetExpression)
DECLARE_TYPE(VariableList)
DECLARE_TYPE(Block)
DECLARE_TYPE(Module)

#ifndef __cplusplus
DECLARE_TYPE(Compiler);
#else
extern "C" {

class Compiler
{
    public:

        // Constructor, destructor.
        Compiler(std::ostream &errorStream, SymbolTable &, Executable &);
        ~Compiler();

        // Compiler methods.
        void LoadApplicationSpec(std::istream &);
        void AddModule(const std::string &);
        void AddModuleFileName(const std::string &);
        std::string NextModuleFileName();
        unsigned ErrorCount() const;
        void Finalize();

#endif

    /* Module (top-level). */
    DECLARE_METHOD
        (MakeModule, NonTerminal *, Block *);

    /* Blocks. */
    DECLARE_METHOD
        (MakeEmptyBlock, Block *, int);
    DECLARE_METHOD
        (AddStatementToBlock, Block *,
         Block *, Statement *);
    DECLARE_METHOD
        (AssignBlock, Block *, Block *);

    /* Statements. */
    DECLARE_METHOD
        (MakeMultipleAssignmentStatement, AssignmentStatement *,
         Token *, Expression *, AssignmentStatement *);
    DECLARE_METHOD
        (MakeSingleAssignmentStatement, AssignmentStatement *,
         Token *, Expression *, Expression *);
    DECLARE_METHOD
        (AssignAssignmentStatement, AssignmentStatement *,
         AssignmentStatement *);
    DECLARE_METHOD
        (MakeAssignmentStatement, Statement *, AssignmentStatement *);
    DECLARE_METHOD
        (MakeMultipleValueInsertionStatement, InsertionStatement *,
         Token *, InsertionStatement *, Expression *);
    DECLARE_METHOD
        (MakeMultiplePairInsertionStatement, InsertionStatement *,
         Token *, InsertionStatement *, KeyValuePair *);
    DECLARE_METHOD
        (MakeSingleValueInsertionStatement, InsertionStatement *,
         Token *, Expression *, Expression *);
    DECLARE_METHOD
        (MakeSinglePairInsertionStatement, InsertionStatement *,
         Token *, Expression *, KeyValuePair *);
    DECLARE_METHOD
        (MakeInsertionStatement, Statement *, InsertionStatement *);
    DECLARE_METHOD
        (MakeExpressionStatement, Statement *,
         Expression *);
    DECLARE_METHOD
        (MakeImportStatement, Statement *, ImportNameList *);
    DECLARE_METHOD
        (MakeFromImportStatement, Statement *,
         ImportName *, ImportNameList *)
    DECLARE_METHOD
        (MakeGlobalStatement, Statement *, VariableList *);
    DECLARE_METHOD
        (MakeLocalStatement, Statement *, VariableList *);
    DECLARE_METHOD
        (MakeDelStatement, Statement *, Expression *);
    DECLARE_METHOD
        (MakeReturnStatement, Statement *,
         Token *, Expression *);
    DECLARE_METHOD
        (MakeAssertStatement, Statement *, Expression *);
    DECLARE_METHOD
        (MakeBreakStatement, Statement *, Token *);
    DECLARE_METHOD
        (MakeContinueStatement, Statement *, Token *);
    DECLARE_METHOD
        (MakePassStatement, Statement *, Token *);
    DECLARE_METHOD
        (MakeIfElseIfStatement, IfStatement *,
         Expression *, Block *, IfStatement *);
    DECLARE_METHOD
        (MakeIfElseStatement, IfStatement *,
         Expression *, Block *, Block *);
    DECLARE_METHOD
        (MakeIfStatement, IfStatement *,
         Expression *, Block *);
    DECLARE_METHOD
        (MakeWhileStatement, Statement *,
         Expression *, Block *, Block *);
    DECLARE_METHOD
        (MakeForStatement, Statement *,
         TargetExpression *, Expression *, Block *, Block *);
    DECLARE_METHOD
        (MakeDefStatement, Statement *,
         Token *, ParameterList *, Block *);
    DECLARE_METHOD
        (AssignStatement, Statement *, Statement *);

    /* Expressions. */
    DECLARE_METHOD
        (MakeConditionalExpression, Expression *,
         Token *, Expression *, Expression *, Expression *);
    DECLARE_METHOD
        (MakeShortCircuitLogicalExpression, Expression *,
         Token *, Expression *, Expression *);
    DECLARE_METHOD
        (MakeBinaryExpression, Expression *,
         Token *, Expression *, Expression *);
    DECLARE_METHOD
        (MakeUnaryExpression, Expression *,
         Token *, Expression *);
    DECLARE_METHOD
        (MakeCallExpression, Expression *,
         Expression *, ArgumentList *);
    DECLARE_METHOD
        (MakeElementExpression, Expression *,
         Expression *, Expression *);
    DECLARE_METHOD
        (MakeMemberExpression, Expression *,
         Expression *, Token *);
    DECLARE_METHOD
        (MakeVariableExpression, Expression *, Token *);
    DECLARE_METHOD
        (MakeSymbolExpression, Expression *,
         Token *, Token *);
    DECLARE_METHOD
        (MakeLiteralExpression, Expression *,
         ConstantExpression *);
    DECLARE_METHOD
        (MakeDictionaryExpression, Expression *, DictionaryExpression *);
    DECLARE_METHOD
        (MakeSetExpression, Expression *, SetExpression *);
    DECLARE_METHOD
        (MakeListExpression, Expression *, ListExpression *);
    DECLARE_METHOD
        (MakeTupleExpression, Expression *,
         Token *, Expression *, Expression *);
    DECLARE_METHOD
        (MakeRangeExpression, Expression *, RangeExpression *);
    DECLARE_METHOD
        (MakeConstantExpression, ConstantExpression *,
         Token *);
    DECLARE_METHOD
        (MakeEnclosedExpression, Expression *, Expression *);
    DECLARE_METHOD
        (AssignExpression, Expression *, Expression *);

    /* Target expressions. */
    DECLARE_METHOD
        (MakeTargetExpression, TargetExpression *,
         Token *, TargetExpression *, TargetExpression *);
    DECLARE_METHOD
        (MakeEnclosedTargetExpression,
         TargetExpression *, TargetExpression *);
    DECLARE_METHOD
        (AssignTargetExpression, TargetExpression *, TargetExpression *);

    /* Imports. */
    DECLARE_METHOD
        (MakeEmptyImportNameList, ImportNameList *, int);
    DECLARE_METHOD
        (AddImportNameToList, ImportNameList *,
         ImportNameList *, ImportName *);
    DECLARE_METHOD
        (MakeImportName, ImportName *,
         Token *, Token *);

    /* Parameters. */
    DECLARE_METHOD
        (MakeEmptyParameterList, ParameterList *, int);
    DECLARE_METHOD
        (AddParameterToList, ParameterList *,
         ParameterList *, Parameter *);
    DECLARE_METHOD
        (MakeParameter, Parameter *,
         Token *, Expression *);
    DECLARE_METHOD
        (MakeTupleGroupParameter, Parameter *,
         Token *);
    DECLARE_METHOD
        (MakeDictionaryGroupParameter, Parameter *,
         Token *);

    /* Arguments. */
    DECLARE_METHOD
        (MakeEmptyArgumentList, ArgumentList *, int);
    DECLARE_METHOD
        (AddArgumentToList, ArgumentList *,
         ArgumentList *, Argument *)
    DECLARE_METHOD
        (MakeArgument, Argument *,
         Token *, Expression *);
    DECLARE_METHOD
        (MakeIterableGroupArgument, Argument *,
         Expression *);
    DECLARE_METHOD
        (MakeDictionaryGroupArgument, Argument *,
         Expression *);

    /* Variables. */
    DECLARE_METHOD
        (MakeEmptyVariableList, VariableList *, int);
    DECLARE_METHOD
        (AddVariableToList, VariableList *,
         VariableList *, Token *);
    DECLARE_METHOD
        (AssignVariableList, VariableList *, VariableList *);

    /* Dictionaries. */
    DECLARE_METHOD
        (MakeEmptyDictionary, DictionaryExpression *, Token *);
    DECLARE_METHOD
        (AddEntryToDictionary, DictionaryExpression *,
         DictionaryExpression *, KeyValuePair *);
    DECLARE_METHOD
        (AssignDictionary, DictionaryExpression *, DictionaryExpression *);

    /* Sets. */
    DECLARE_METHOD
        (MakeEmptySet, SetExpression *, Token *);
    DECLARE_METHOD
        (AssignSet, SetExpression *, Token *, ListExpression *);

    /* Lists. */
    DECLARE_METHOD
        (MakeEmptyList, ListExpression *, Token *);
    DECLARE_METHOD
        (AddExpressionToList, ListExpression *,
         ListExpression *, Expression *);
    DECLARE_METHOD
        (AssignList, ListExpression *, Token *, ListExpression *);

    /* Key/value pairs. */
    DECLARE_METHOD
        (MakeKeyValuePair, KeyValuePair *,
         Expression *, Expression *);

    /* Ranges. */
    DECLARE_METHOD
        (MakeRange, RangeExpression *,
         Token *, Expression *, Expression *, Expression *);

    /* Tokens. */
    DECLARE_METHOD
        (AssignToken, Token *, Token *);

    /* Miscellaneous methods. */
    DECLARE_METHOD(FreeNonTerminal, void, NonTerminal *);
    DECLARE_METHOD(FreeToken, void, Token *);
    DECLARE_METHOD(ReportError, void, const char *);

#ifdef __cplusplus

    protected:

        // Copy prevention.
        Compiler(const Compiler &) = delete;
        Compiler &operator =(const Compiler &) = delete;

        // Error reporting methods.
        void ReportError(const std::string &);
        void ReportError(const std::string &, const SourceElement &);
        void ReportError(const std::string &, const SourceLocation &);

    private:

        // Error reporting data.
        std::ostream &errorStream;
        unsigned errorCount = 0;

        // Code generation data.
        SourceLocation currentSourceLocation;
        SymbolTable &symbolTable;
        Executable &executable;
        Executable::Location topLocation;

        // Module data.
        std::set<std::string> moduleNames;
        std::string topModuleName;
        std::deque<std::string> moduleNamesToImport;
        std::string currentModuleName;
        std::int32_t currentModuleSymbol;
};

} // extern "C"

#endif

#endif
