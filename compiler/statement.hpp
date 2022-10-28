//
// Asp statement definitions.
//

#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "grammar.hpp"
#include "expression.hpp"
#include "executable.hpp"
#include <list>
#include <string>

class Block;
class LoopStatement;
class DefStatement;

class Statement : public NonTerminal
{
    protected:

        explicit Statement(const SourceElement &);

    public:

        virtual void Parent(const Block *);
        const Block *Parent() const;

        virtual void Emit(Executable &) const = 0;

        const LoopStatement *ParentLoop() const;
        const DefStatement *ParentDef() const;

    private:

        const Block *parentBlock = 0;
};

class Block : public NonTerminal
{
    public:

        Block();
        ~Block();

        void Add(Statement *);

        void Parent(Statement *);
        const Statement *Parent() const;

        const Statement *FinalStatement() const;

        void Emit(Executable &) const;

    private:

        Statement *parentStatement = 0;
        std::list<Statement *> statements;
};

class ExpressionStatement : public Statement
{
    public:

        explicit ExpressionStatement(Expression *);
        ~ExpressionStatement();

        virtual void Emit(Executable &) const;

    protected:

        Expression *expression;
};

class AssignmentStatement : public Statement
{

    public:

        AssignmentStatement
            (const Token &assignmentToken,
             Expression *target, AssignmentStatement *);
        AssignmentStatement
            (const Token &assignmentToken,
             Expression *target, Expression *value);
        ~AssignmentStatement();

        virtual void Parent(const Block *);

        virtual void Emit(Executable &) const;
        void Emit1(Executable &, bool top) const;

    private:

        int assignmentTokenType;
        Expression *targetExpression, *valueExpression;
        AssignmentStatement *valueAssignmentStatement;
};

class InsertionStatement : public Statement
{
    public:

        InsertionStatement
            (const Token &insertionToken,
             InsertionStatement *, Expression *item);
        InsertionStatement
            (const Token &insertionToken,
             InsertionStatement *, KeyValuePair *item);
        InsertionStatement
            (const Token &insertionToken,
             Expression *container, Expression *item);
        InsertionStatement
            (const Token &insertionToken,
             Expression *container, KeyValuePair *);
        ~InsertionStatement();

        virtual void Emit(Executable &) const;
        void Emit1(Executable &, bool top) const;

    private:

        InsertionStatement *containerInsertionStatement;
        Expression *containerExpression, *itemExpression;
        KeyValuePair *keyValuePair;
};

class BreakStatement : public Statement
{
    public:

        explicit BreakStatement(const Token &token);

        virtual void Emit(Executable &) const;
};

class ContinueStatement : public Statement
{
    public:

        explicit ContinueStatement(const Token &token);

        virtual void Emit(Executable &) const;
};

class PassStatement : public Statement
{
    public:

        explicit PassStatement(const Token &token);

        virtual void Emit(Executable &) const;
};

class ImportName : public NonTerminal
{
    public:

        explicit ImportName(const Token &name);
        ImportName(const Token &name, const Token &asName);

        std::string Name() const
        {
            return name;
        }
        std::string AsName() const
        {
            return asName.empty() ? name : asName;
        }

    private:

        std::string name, asName;
};

class ImportNameList : public NonTerminal
{
    public:

        ImportNameList();
        ~ImportNameList();

        void Add(ImportName *);

        typedef std::list<ImportName *>::const_iterator ConstNameIterator;

        ConstNameIterator NamesBegin() const
        {
            return names.begin();
        }
        ConstNameIterator NamesEnd() const
        {
            return names.end();
        }

    private:

        std::list<ImportName *> names;
};

class ImportStatement : public Statement
{
    public:

        explicit ImportStatement
            (ImportNameList *moduleNameList,
             ImportNameList *memberNameList = 0);
        ~ImportStatement();

        virtual void Emit(Executable &) const;

    private:

        ImportNameList *moduleNameList, *memberNameList;
};

class VariableList : public NonTerminal
{
    public:

        VariableList();

        void Add(const Token &);

        void Parent(const Statement *);

        typedef std::list<std::string>::const_iterator ConstNameIterator;
        std::size_t NamesSize() const
        {
            return names.size();
        }
        ConstNameIterator NamesBegin() const
        {
            return names.begin();
        }
        ConstNameIterator NamesEnd() const
        {
            return names.end();
        }

        void Emit(Executable &) const;

    private:

        const Statement *parentStatement;
        std::list<std::string> names;
};

class GlobalStatement : public Statement
{
    public:

        explicit GlobalStatement(VariableList *);
        ~GlobalStatement();

        virtual void Emit(Executable &) const;

    private:

        VariableList *variableList;
};

class LocalStatement : public Statement
{
    public:

        explicit LocalStatement(VariableList *);
        ~LocalStatement();

        virtual void Emit(Executable &) const;

    private:

        VariableList *variableList;
};

class DelStatement : public ExpressionStatement
{
    public:

        explicit DelStatement(Expression *);

        virtual void Emit(Executable &) const;
        void Emit1(Executable &, Expression *) const;
};

class ReturnStatement : public Statement
{
    public:

        ReturnStatement(const Token &keywordToken, Expression *);
        ~ReturnStatement();

        virtual void Emit(Executable &) const;

    private:

        Expression *expression;
};

class IfStatement : public Statement
{
    public:

        IfStatement(Expression *, Block *, IfStatement *);
        IfStatement(Expression *, Block *, Block *);
        IfStatement(Expression *, Block *);
        ~IfStatement();

        virtual void Parent(const Block *);

        virtual void Emit(Executable &) const;

    private:

        Expression *conditionExpression;
        Block *trueBlock, *falseBlock;
        IfStatement *elsePart;
};

class LoopStatement : public Statement
{
    public:

        Executable::Location ContinueLocation() const;
        Executable::Location EndLocation() const;

    protected:

        explicit LoopStatement(const SourceElement &);

        mutable Executable::Location continueLocation, endLocation;
};

class WhileStatement : public LoopStatement
{
    public:

        WhileStatement(Expression *, Block *, Block *);
        ~WhileStatement();

        virtual void Emit(Executable &) const;

    private:

        Expression *conditionExpression;
        Block *trueBlock, *falseBlock;
};

class ForStatement : public LoopStatement
{
    public:

        ForStatement(VariableList *, Expression *, Block *, Block *);
        ~ForStatement();

        virtual void Emit(Executable &) const;

    private:

        VariableList *variableList;
        Expression *iterableExpression;
        Block *trueBlock, *falseBlock;
};

class Parameter : public NonTerminal
{
    public:

        Parameter(const Token &name, Expression *, bool group = false);
        ~Parameter();

        void Parent(const Statement *);

        bool HasDefault() const
        {
            return defaultExpression != 0;
        }
        bool IsGroup() const
        {
            return isGroup;
        }

        void Emit(Executable &) const;

    private:

        std::string name;
        Expression *defaultExpression;
        bool isGroup;
};

class ParameterList : public NonTerminal
{
    public:

        ParameterList();
        ~ParameterList();

        void Add(Parameter *);

        void Parent(const Statement *);

        typedef std::list<Parameter *>::const_iterator ConstParameterIterator;
        ConstParameterIterator ParametersBegin() const
        {
            return parameters.begin();
        }
        ConstParameterIterator ParametersEnd() const
        {
            return parameters.end();
        }

        void Emit(Executable &) const;

    private:

        std::list<Parameter *> parameters;
};

class DefStatement : public Statement
{
    public:

        DefStatement(const Token &nameToken, ParameterList *, Block *);
        ~DefStatement();

        virtual void Emit(Executable &) const;

    private:

        std::string name;
        ParameterList *parameterList;
        Block *block;
};

#endif
