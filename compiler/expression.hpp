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

class TernaryExpression : public Expression
{
    public:

        TernaryExpression
            (const Token &operatorToken,
             Expression *, Expression *, Expression *);
        ~TernaryExpression();

        virtual void Parent(const Statement *);

        virtual void Emit(Executable &, EmitType) const;

    private:

        int operatorTokenType;
        Expression *conditionExpression, *trueExpression, *falseExpression;
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
        explicit Argument(Expression *);
        ~Argument();

        void Parent(const Statement *);

        bool HasName() const
        {
            return !name.empty();
        }

        void Emit(Executable &) const;

    private:

        std::string name;
        Expression *valueExpression;
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

class ConstantExpression : public Expression
{
    public:

        explicit ConstantExpression(const Token &);

        ConstantExpression *FoldUnary(int operatorTokenType) const;
        ConstantExpression *FoldBinary
            (int operatorTokenType, const ConstantExpression *) const;
        ConstantExpression *FoldTernary
            (int operatorTokenType,
             const ConstantExpression *, const ConstantExpression *) const;

        virtual void Emit(Executable &, EmitType) const;

    protected:

        ConstantExpression *Not() const;
        ConstantExpression *Plus() const;
        ConstantExpression *Minus() const;
        ConstantExpression *Invert() const;
        ConstantExpression *Or(const ConstantExpression *) const;
        ConstantExpression *And(const ConstantExpression *) const;
        ConstantExpression *Equal(const ConstantExpression *) const;
        ConstantExpression *NotEqual(const ConstantExpression *) const;
        ConstantExpression *Less(const ConstantExpression *) const;
        ConstantExpression *LessOrEqual(const ConstantExpression *) const;
        ConstantExpression *Greater(const ConstantExpression *) const;
        ConstantExpression *GreaterOrEqual(const ConstantExpression *) const;
        ConstantExpression *BitOr(const ConstantExpression *) const;
        ConstantExpression *BitExclusiveOr(const ConstantExpression *) const;
        ConstantExpression *BitAnd(const ConstantExpression *) const;
        ConstantExpression *LeftShift(const ConstantExpression *) const;
        ConstantExpression *RightShift(const ConstantExpression *) const;
        ConstantExpression *Multiply(const ConstantExpression *) const;
        ConstantExpression *Divide(const ConstantExpression *) const;
        ConstantExpression *FloorDivide(const ConstantExpression *) const;
        ConstantExpression *Modulo(const ConstantExpression *) const;
        ConstantExpression *Power(const ConstantExpression *) const;
        ConstantExpression *Conditional
            (const ConstantExpression *, const ConstantExpression *) const;

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
