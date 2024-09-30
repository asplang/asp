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
        virtual const Block *Parent() const final;
        virtual unsigned StackUsage() const;

        virtual void Emit(Executable &) const = 0;

        const LoopStatement *ParentLoop() const;
        const DefStatement *ParentDef() const;

    private:

        const Block *parentBlock = nullptr;
};

class Block : public NonTerminal
{
    public:

        ~Block() override;

        void Add(Statement *);

        void Parent(const Statement *);
        const Statement *Parent() const;

        const Statement *FinalStatement() const;

        void Emit(Executable &) const;

    private:

        const Statement *parentStatement = nullptr;
        std::list<Statement *> statements;
};

class ExpressionStatement : public Statement
{
    public:

        explicit ExpressionStatement(Expression *);
        ~ExpressionStatement() override;

        void Emit(Executable &) const override;

        const Expression *GetExpression() const
        {
            return expression;
        }

    private:

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
        ~AssignmentStatement() override;

        void Parent(const Block *) override;

        void Emit(Executable &) const override;
        void Emit1(Executable &, bool top) const;

    private:

        int assignmentTokenType;
        Expression *targetExpression, *valueExpression = nullptr;
        AssignmentStatement *valueAssignmentStatement = nullptr;
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
        ~InsertionStatement() override;

        void Emit(Executable &) const override;
        void Emit1(Executable &, bool top) const;

    private:

        InsertionStatement *containerInsertionStatement = nullptr;
        Expression *containerExpression = nullptr, *itemExpression = nullptr;
        KeyValuePair *keyValuePair = nullptr;
};

class BreakStatement : public Statement
{
    public:

        explicit BreakStatement(const Token &token);

        void Emit(Executable &) const override;
};

class ContinueStatement : public Statement
{
    public:

        explicit ContinueStatement(const Token &token);

        void Emit(Executable &) const override;
};

class PassStatement : public Statement
{
    public:

        explicit PassStatement(const Token &token);

        void Emit(Executable &) const override;
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

        ~ImportNameList() override;

        void Add(ImportName *);

        using ConstNameIterator = std::list<ImportName *>::const_iterator;
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

    private:

        std::list<ImportName *> names;
};

class ImportStatement : public Statement
{
    public:

        explicit ImportStatement
            (ImportNameList *moduleNameList,
             ImportNameList *memberNameList = nullptr);
        ~ImportStatement() override;

        void Emit(Executable &) const override;

    private:

        ImportNameList *moduleNameList, *memberNameList;
};

class VariableList : public NonTerminal
{
    public:

        void Add(const Token &);

        void Parent(const Statement *);

        using ConstNameIterator = std::list<std::string>::const_iterator;
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

    private:

        const Statement *parentStatement = nullptr;
        std::list<std::string> names;
};

class GlobalStatement : public Statement
{
    public:

        explicit GlobalStatement(VariableList *);
        ~GlobalStatement() override;

        void Emit(Executable &) const override;

    private:

        VariableList *variableList;
};

class LocalStatement : public Statement
{
    public:

        explicit LocalStatement(VariableList *);
        ~LocalStatement() override;

        void Emit(Executable &) const override;

    private:

        VariableList *variableList;
};

class DelStatement : public ExpressionStatement
{
    public:

        explicit DelStatement(Expression *);

        void Emit(Executable &) const override;
        void Emit1(Executable &, const Expression *) const;
};

class ReturnStatement : public Statement
{
    public:

        ReturnStatement(const Token &keywordToken, Expression *);
        ~ReturnStatement() override;

        void Emit(Executable &) const override;

    private:

        Expression *expression;
};

class AssertStatement : public ExpressionStatement
{
    public:

        explicit AssertStatement(Expression *);

        void Emit(Executable &) const override;
};

class IfStatement : public Statement
{
    public:

        IfStatement(Expression *, Block *, IfStatement *);
        IfStatement(Expression *, Block *, Block *);
        IfStatement(Expression *, Block *);
        ~IfStatement() override;

        void Parent(const Block *) override;

        void Emit(Executable &) const override;

    private:

        Expression *conditionExpression;
        Block *trueBlock, *falseBlock = nullptr;
        IfStatement *elsePart = nullptr;
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
        ~WhileStatement() override;

        void Emit(Executable &) const override;

    private:

        Expression *conditionExpression;
        Block *trueBlock, *falseBlock;
};

class ForStatement : public LoopStatement
{
    public:

        ForStatement
            (TargetExpression *, Expression *,
             Block *, Block *);
        ~ForStatement() override;

        unsigned StackUsage() const override;

        void Emit(Executable &) const override;

    private:

        TargetExpression *targetExpression;
        Expression *iterableExpression;
        Block *trueBlock, *falseBlock;
};

class Parameter : public NonTerminal
{
    public:

        enum class Type
        {
            Positional,
            TupleGroup,
            DictionaryGroup,
        };

        explicit Parameter
            (const Token &name, Type = Type::Positional,
             Expression * = nullptr);
        ~Parameter() override;

        void Parent(const Statement *) const;

        const std::string &Name() const
        {
            return name;
        }
        Type GetType() const
        {
            return type;
        }
        bool HasDefault() const
        {
            return defaultExpression != nullptr;
        }

        void Emit(Executable &) const;

    private:

        std::string name;
        Type type;
        Expression *defaultExpression;
};

class ParameterList : public NonTerminal
{
    public:

        ~ParameterList() override;

        void Add(Parameter *);

        void Parent(const Statement *) const;

        using ConstParameterIterator = std::list<Parameter *>::const_iterator;
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
        ~DefStatement() override;

        void Emit(Executable &) const override;

    private:

        std::string name;
        ParameterList *parameterList;
        Block *block;
};

#endif
