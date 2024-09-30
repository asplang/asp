//
// Asp expression definitions.
//

#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include "grammar.hpp"
#include "token.h"
#include "executable.hpp"
#include <list>
#include <string>
#include <cstdint>

class Statement;

class Expression : public NonTerminal
{
    protected:

        explicit Expression(const SourceElement & = SourceElement());

    public:

        void Enclose();
        bool IsEnclosed() const;

        virtual void Parent(const Statement *);
        virtual const Statement *Parent() const final;

        enum class EmitType
        {
            Value,
            Address,
            Delete,
        };

        virtual void Emit(Executable &, EmitType = EmitType::Value) const = 0;

    private:

        bool enclosed = false;
        const Statement *parentStatement = nullptr;
};

class ConditionalExpression : public Expression
{
    public:

        ConditionalExpression
            (const Token &operatorToken,
             Expression *, Expression *, Expression *);
        ~ConditionalExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        int operatorTokenType;
        Expression *conditionExpression, *trueExpression, *falseExpression;
};

class ShortCircuitLogicalExpression : public Expression
{
    public:

        ShortCircuitLogicalExpression
            (const Token &operatorToken, Expression *, Expression *);
        ~ShortCircuitLogicalExpression() override;

        void Add(Expression *);

        void Parent(const Statement *) override;

        int OperatorTokenType() const
        {
            return operatorTokenType;
        }

        void Emit(Executable &, EmitType) const override;

    private:

        int operatorTokenType;
        std::list<Expression *> expressions;
};

class BinaryExpression : public Expression
{
    public:

        BinaryExpression
            (const Token &operatorToken, Expression *, Expression *);
        ~BinaryExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        int operatorTokenType;
        Expression *leftExpression, *rightExpression;
};

class UnaryExpression : public Expression
{
    public:

        UnaryExpression(const Token &operatorToken, Expression *);
        ~UnaryExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        int operatorTokenType;
        Expression *expression;
};

class TargetExpression : public Expression
{
    public:

        TargetExpression() = default;
        explicit TargetExpression(const Token &nameToken);
        ~TargetExpression() override;

        bool IsTuple() const;
        void Add(TargetExpression *);

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        const Statement *parentStatement = nullptr;
        std::string name;
        std::list<TargetExpression *> targetExpressions;
};

class Argument : public NonTerminal
{
    public:

        enum class Type
        {
            NonGroup,
            IterableGroup,
            DictionaryGroup,
        };

        Argument(const Token &, Expression *);
        explicit Argument(Expression *, Type = Type::NonGroup);
        ~Argument() override;

        void Parent(const Statement *) const;

        Type GetType() const
        {
            return type;
        }
        bool HasName() const
        {
            return !name.empty();
        }

        void Emit(Executable &) const;

    private:

        Type type;
        std::string name;
        Expression *valueExpression;
};

class ArgumentList : public NonTerminal
{
    public:

        ~ArgumentList() override;

        void Add(Argument *);

        void Parent(const Statement *) const;

        using ConstArgumentIterator =
            std::list<Argument *>::const_iterator;
        ConstArgumentIterator ArgumentsBegin() const
        {
            return arguments.begin();
        }
        ConstArgumentIterator ArgumentsEnd() const
        {
            return arguments.end();
        }

        void Emit(Executable &) const;

    private:

        std::list<Argument *> arguments;
};

class CallExpression : public Expression
{
    public:

        CallExpression(Expression *, ArgumentList *);
        ~CallExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        Expression *functionExpression;
        ArgumentList *argumentList;
};

class ElementExpression : public Expression
{
    public:

        ElementExpression(Expression *, Expression *);
        ~ElementExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        Expression *sequenceExpression, *indexExpression;
};

class MemberExpression : public Expression
{
    public:

        MemberExpression(Expression *, const Token &);
        ~MemberExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        Expression *expression;
        std::string name;
};

class VariableExpression : public Expression
{
    public:

        explicit VariableExpression(const Token &nameToken);
        VariableExpression(const SourceElement &, int32_t symbol);

        bool HasSymbol() const;
        std::string Name() const;

        void Emit(Executable &, EmitType) const override;

    private:

        std::string name;
        bool hasSymbol;
        int32_t symbol;
};

class SymbolExpression : public Expression
{
    public:

        SymbolExpression
            (const Token &operatorToken, const Token &nameToken);

        void Emit(Executable &, EmitType) const override;

    private:

        std::string name;
};

class KeyValuePair : public NonTerminal
{
    public:

        KeyValuePair(Expression *key, Expression *value);
        ~KeyValuePair() override;

        void Parent(const Statement *) const;

        void Emit(Executable &) const;

    private:

        Expression *keyExpression, *valueExpression;
};

class DictionaryExpression : public Expression
{
    public:

        DictionaryExpression() = default;
        explicit DictionaryExpression(const Token &);
        ~DictionaryExpression() override;

        void Add(KeyValuePair *);

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        std::list<KeyValuePair *> entries;
};

class SetExpression : public Expression
{
    public:

        explicit SetExpression(const Token &);
        ~SetExpression() override;

        void Add(Expression *);

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        std::list<Expression *> expressions;
};

class ListExpression : public Expression
{
    public:

        ListExpression() = default;
        explicit ListExpression(const Token &);
        ~ListExpression() override;

        void Add(Expression *);
        Expression *PopFront();
        bool IsEmpty() const;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        std::list<Expression *> expressions;
};

class TupleExpression : public Expression
{
    public:

        explicit TupleExpression(const Token &);
        ~TupleExpression() override;

        void Add(Expression *);

        void Parent(const Statement *) override;

        using ConstExpressionIterator =
            std::list<Expression *>::const_iterator;
        ConstExpressionIterator ExpressionsBegin() const
        {
            return expressions.begin();
        }
        ConstExpressionIterator ExpressionsEnd() const
        {
            return expressions.end();
        }

        void Emit(Executable &, EmitType) const override;

    private:

        std::list<Expression *> expressions;
};

class RangeExpression : public Expression
{
    public:

        RangeExpression
            (const Token &, Expression *, Expression *, Expression *);
        ~RangeExpression() override;

        void Parent(const Statement *) override;

        void Emit(Executable &, EmitType) const override;

    private:

        Expression *startExpression, *endExpression, *stepExpression;
};

Expression *FoldUnaryExpression
    (int operatorTokenType, Expression *);
Expression *FoldBinaryExpression
    (int operatorTokenType, Expression *, Expression *);
Expression *FoldTernaryExpression
    (int operatorTokenType, const Expression *, Expression *, Expression *);

class ConstantExpression : public Expression
{
    public:

        explicit ConstantExpression(const Token &);

        enum class Type
        {
            None,
            Ellipsis,
            Boolean,
            Integer,
            NegatedMinInteger,
            Float,
            String,
        };

        Type GetType() const
        {
            return type;
        }

        friend Expression *FoldUnaryExpression
            (int operatorTokenType, Expression *);
        friend Expression *FoldBinaryExpression
            (int operatorTokenType, Expression *, Expression *);
        friend Expression *FoldTernaryExpression
            (int operatorTokenType, Expression *, Expression *, Expression *);

        void Emit(Executable &, EmitType) const override;

        bool IsTrue() const;
        bool IsString() const;

    protected:

        bool IsEqual(const ConstantExpression &) const;
        int Compare(const ConstantExpression &) const;
        int NumericCompare(const ConstantExpression &) const;
        int StringCompare(const ConstantExpression &) const;

        Expression *FoldNot() const;
        Expression *FoldPlus();
        Expression *FoldMinus() const;
        Expression *FoldInvert() const;
        friend Expression *FoldOr(ConstantExpression *, Expression *);
        friend Expression *FoldAnd(ConstantExpression *, Expression *);
        friend Expression *FoldEqual
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldNotEqual
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldLess
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldLessOrEqual
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldGreater
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldGreaterOrEqual
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldObjectOrder
            (const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldBitwiseOperation
            (int operatorTypeType,
             const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldStringConcatenationOperation
            (int operatorTypeType, ConstantExpression *, ConstantExpression *);
        friend Expression *FoldArithmeticOperation
            (int operatorTypeType,
             const ConstantExpression *, const ConstantExpression *);
        friend Expression *FoldConditional
            (const ConstantExpression *, Expression *, Expression *);

    private:

        Type type;
        union
        {
            bool b;
            std::int32_t i;
            double f;
        };
        std::string s;
};

#endif
