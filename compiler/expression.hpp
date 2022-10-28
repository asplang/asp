//
// Asp expression definitions.
//

#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include "grammar.hpp"
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

        virtual void Parent(const Statement *);
        const Statement *Parent() const;

        enum class EmitType
        {
            Value,
            Address,
            Delete,
        };

        virtual void Emit(Executable &, EmitType = EmitType::Value) const = 0;

    private:

        const Statement *parentStatement;
};

class ConditionalExpression : public Expression
{
    public:

        ConditionalExpression
            (const Token &operatorToken,
             Expression *, Expression *, Expression *);
        ~ConditionalExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        int operatorTokenType;
        Expression *conditionExpression, *trueExpression, *falseExpression;
};

class ShortCircuitLogicalExpression : public Expression
{
    public:

        ShortCircuitLogicalExpression
            (const Token &operatorToken, Expression *, Expression *);
        ~ShortCircuitLogicalExpression();

        void Add(Expression *);

        virtual void Parent(const Statement *);

        int OperatorTokenType() const
        {
            return operatorTokenType;
        }

        virtual void Emit(Executable &, EmitType) const;

    private:

        int operatorTokenType;
        std::list<Expression *> expressions;
};

class BinaryExpression : public Expression
{
    public:

        BinaryExpression
            (const Token &operatorToken, Expression *, Expression *);
        ~BinaryExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        int operatorTokenType;
        Expression *leftExpression, *rightExpression;
};

class UnaryExpression : public Expression
{
    public:

        UnaryExpression(const Token &operatorToken, Expression *);
        ~UnaryExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        int operatorTokenType;
        Expression *expression;
};

class Argument : public NonTerminal
{
    public:

        Argument(const Token &, Expression *);
        explicit Argument(Expression *, bool isGroup = false);
        ~Argument();

        void Parent(const Statement *);

        bool HasName() const
        {
            return !name.empty();
        }
        bool IsGroup() const
        {
            return isGroup;
        }

        void Emit(Executable &) const;

    private:

        std::string name;
        Expression *valueExpression;
        bool isGroup;
};

class ArgumentList : public NonTerminal
{
    public:

        ArgumentList();
        ~ArgumentList();

        void Add(Argument *);

        void Parent(const Statement *);

        typedef std::list<Argument *>::const_iterator ConstArgumentIterator;
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
        ~CallExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        Expression *functionExpression;
        ArgumentList *argumentList;
};

class ElementExpression : public Expression
{
    public:

        ElementExpression(Expression *, Expression *);
        ~ElementExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        Expression *sequenceExpression, *indexExpression;
};

class MemberExpression : public Expression
{
    public:

        MemberExpression(Expression *, const Token &);
        ~MemberExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

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

        virtual void Emit(Executable &, EmitType) const;

    private:

        std::string name;
        bool hasSymbol;
        int32_t symbol;
};

class KeyValuePair : public NonTerminal
{
    public:

        KeyValuePair(Expression *key, Expression *value);
        ~KeyValuePair();

        void Parent(const Statement *);

        void Emit(Executable &) const;

    private:

        Expression *keyExpression, *valueExpression;
};

class DictionaryExpression : public Expression
{
    public:

        DictionaryExpression(const Token &);
        DictionaryExpression();
        ~DictionaryExpression();

        void Add(KeyValuePair *);

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        std::list<KeyValuePair *> entries;
};

class SetExpression : public Expression
{
    public:

        SetExpression();
        ~SetExpression();

        void Add(Expression *);
        bool Empty() const;

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        std::list<Expression *> expressions;
};

class ListExpression : public Expression
{
    public:

        ListExpression();
        ~ListExpression();

        void Add(Expression *);
        bool Empty() const;

        virtual void Parent(const Statement *);

        typedef std::list<Expression *>::const_iterator ConstExpressionIterator;
        ConstExpressionIterator ExpressionsBegin() const
        {
            return expressions.begin();
        }
        ConstExpressionIterator ExpressionsEnd() const
        {
            return expressions.end();
        }

        virtual void Emit(Executable &, EmitType) const;

    private:

        std::list<Expression *> expressions;
};

class TupleExpression : public Expression
{
    public:

        TupleExpression(const Token &);
        ~TupleExpression();

        void Add(Expression *);
        Expression *Remove();
        bool Empty() const;

        virtual void Parent(const Statement *);

        typedef std::list<Expression *>::const_iterator ConstExpressionIterator;
        ConstExpressionIterator ExpressionsBegin() const
        {
            return expressions.begin();
        }
        ConstExpressionIterator ExpressionsEnd() const
        {
            return expressions.end();
        }

        virtual void Emit(Executable &, EmitType) const;

    private:

        std::list<Expression *> expressions;
};

class RangeExpression : public Expression
{
    public:

        RangeExpression
            (const Token &, Expression *, Expression *, Expression *);
        ~RangeExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        Expression *startExpression, *endExpression, *stepExpression;
};

Expression *FoldUnaryExpression
    (int operatorTokenType, Expression *);
Expression *FoldBinaryExpression
    (int operatorTokenType, Expression *, Expression *);
Expression *FoldTernaryExpression
    (int operatorTokenType, Expression *, Expression *, Expression *);

class ConstantExpression : public Expression
{
    public:

        explicit ConstantExpression(const Token &);

        friend Expression *FoldUnaryExpression
            (int operatorTokenType, Expression *);
        friend Expression *FoldBinaryExpression
            (int operatorTokenType, Expression *, Expression *);
        friend Expression *FoldTernaryExpression
            (int operatorTokenType, Expression *, Expression *, Expression *);

        virtual void Emit(Executable &, EmitType) const;

    protected:

        bool IsTrue() const;
        bool IsEqual(const ConstantExpression &) const;
        int Compare(const ConstantExpression &) const;
        int NumericCompare(const ConstantExpression &) const;
        int StringCompare(const ConstantExpression &) const;

        Expression *FoldNot();
        Expression *FoldPlus();
        Expression *FoldMinus();
        Expression *FoldInvert();
        friend Expression *FoldOr(ConstantExpression *, Expression *);
        friend Expression *FoldAnd(ConstantExpression *, Expression *);
        friend Expression *FoldEqual
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldNotEqual
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldLess
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldLessOrEqual
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldGreater
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldGreaterOrEqual
            (ConstantExpression *, ConstantExpression *);
        friend Expression *FoldBitwiseOperation
            (int operatorTypeType, ConstantExpression *, ConstantExpression *);
        friend Expression *FoldArithmeticOperation
            (int operatorTypeType, ConstantExpression *, ConstantExpression *);
        friend Expression *FoldConditional
            (ConstantExpression *, Expression *, Expression *);

        enum class Type
        {
            None,
            Ellipsis,
            Boolean,
            Integer,
            Float,
            String,
        };

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
