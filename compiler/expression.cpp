//
// Asp expression implementation.
//

#include "expression.hpp"
#include "asp.h"

using namespace std;

Expression::Expression(const SourceElement &sourceElement) :
    NonTerminal(sourceElement)
{
}

void Expression::Parent(const Statement *statement)
{
    parentStatement = statement;
}

const Statement *Expression::Parent() const
{
    return parentStatement;
}

TernaryExpression::TernaryExpression
    (const Token &operatorToken, Expression *conditionExpression,
     Expression *trueExpression, Expression *falseExpression) :
    Expression(operatorToken),
    operatorTokenType(operatorToken.type),
    conditionExpression(conditionExpression),
    trueExpression(trueExpression),
    falseExpression(falseExpression)
{
}

TernaryExpression::~TernaryExpression()
{
    delete conditionExpression;
    delete trueExpression;
    delete falseExpression;
}

void TernaryExpression::Parent(const Statement *statement)
{
    conditionExpression->Parent(statement);
    trueExpression->Parent(statement);
    falseExpression->Parent(statement);
}

BinaryExpression::BinaryExpression
    (const Token &operatorToken,
     Expression *leftExpression, Expression *rightExpression) :
    Expression(operatorToken),
    operatorTokenType(operatorToken.type),
    leftExpression(leftExpression),
    rightExpression(rightExpression)
{
}

BinaryExpression::~BinaryExpression()
{
    delete leftExpression;
    delete rightExpression;
}

void BinaryExpression::Parent(const Statement *statement)
{
    leftExpression->Parent(statement);
    rightExpression->Parent(statement);
}

UnaryExpression::UnaryExpression
    (const Token &operatorToken, Expression *expression) :
    Expression(operatorToken),
    operatorTokenType(operatorToken.type),
    expression(expression)
{
}

UnaryExpression::~UnaryExpression()
{
    delete expression;
}

void UnaryExpression::Parent(const Statement *statement)
{
    expression->Parent(statement);
}

Argument::Argument(const Token &nameToken, Expression *valueExpression) :
    NonTerminal(nameToken),
    name(nameToken.s),
    valueExpression(valueExpression)
{
}

Argument::Argument(Expression *valueExpression) :
    NonTerminal((SourceElement &)*valueExpression),
    valueExpression(valueExpression)
{
}

Argument::~Argument()
{
    delete valueExpression;
}

void Argument::Parent(const Statement *statement)
{
    valueExpression->Parent(statement);
}

ArgumentList::ArgumentList()
{
}

ArgumentList::~ArgumentList()
{
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
        delete *iter;
}

void ArgumentList::Add(Argument *argument)
{
    if (arguments.empty())
        (SourceElement &)*this = *argument;
    arguments.push_back(argument);
}

void ArgumentList::Parent(const Statement *statement)
{
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        auto argument = *iter;
        argument->Parent(statement);
    }
}

CallExpression::CallExpression
    (Expression *functionExpression, ArgumentList *argumentList) :
    functionExpression(functionExpression),
    argumentList(argumentList)
{
}

CallExpression::~CallExpression()
{
    delete functionExpression;
    delete argumentList;
}

void CallExpression::Parent(const Statement *statement)
{
    functionExpression->Parent(statement);
    argumentList->Parent(statement);
}

ElementExpression::ElementExpression
    (Expression *sequenceExpression, Expression *indexExpression) :
    sequenceExpression(sequenceExpression),
    indexExpression(indexExpression)
{
}

ElementExpression::~ElementExpression()
{
    delete sequenceExpression;
    delete indexExpression;
}

void ElementExpression::Parent(const Statement *statement)
{
    sequenceExpression->Parent(statement);
    indexExpression->Parent(statement);
}

MemberExpression::MemberExpression
    (Expression *expression, const Token &nameToken) :
    Expression((SourceElement &)*expression),
    expression(expression),
    name(nameToken.s)
{
}

MemberExpression::~MemberExpression()
{
    delete expression;
}

void MemberExpression::Parent(const Statement *statement)
{
    expression->Parent(statement);
}

VariableExpression::VariableExpression(const Token &nameToken) :
    Expression(nameToken),
    name(nameToken.s),
    hasSymbol(false), symbol(0)
{
}

VariableExpression::VariableExpression
    (const SourceElement &sourceElement, int symbol) :
    Expression(sourceElement),
    hasSymbol(true), symbol(symbol)
{
}

bool VariableExpression::HasSymbol() const
{
    return hasSymbol;
}

string VariableExpression::Name() const
{
    return name;
}

KeyValuePair::KeyValuePair
    (Expression *keyExpression, Expression *valueExpression) :
    NonTerminal((SourceElement &)*keyExpression),
    keyExpression(keyExpression),
    valueExpression(valueExpression)
{
}

KeyValuePair::~KeyValuePair()
{
    delete keyExpression;
    delete valueExpression;
}

void KeyValuePair::Parent(const Statement *statement)
{
    keyExpression->Parent(statement);
    valueExpression->Parent(statement);
}

DictionaryExpression::DictionaryExpression(const Token &token) :
    Expression(token)
{
}

DictionaryExpression::DictionaryExpression()
{
}

DictionaryExpression::~DictionaryExpression()
{
    for (auto iter = entries.begin(); iter != entries.end(); iter++)
        delete *iter;
}

void DictionaryExpression::Add(KeyValuePair *entry)
{
    if (entries.empty())
        (SourceElement &)*this = *entry;
    entries.push_back(entry);
}

void DictionaryExpression::Parent(const Statement *statement)
{
    for (auto iter = entries.begin(); iter != entries.end(); iter++)
    {
        auto entry = *iter;
        entry->Parent(statement);
    }
}

SetExpression::SetExpression()
{
}

SetExpression::~SetExpression()
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
        delete *iter;
}

void SetExpression::Add(Expression *expression)
{
    if (expressions.empty())
        (SourceElement &)*this = *expression;
    expressions.push_back(expression);
}

bool SetExpression::Empty() const
{
    return expressions.empty();
}

void SetExpression::Parent(const Statement *statement)
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Parent(statement);
    }
}

ListExpression::ListExpression()
{
}

ListExpression::~ListExpression()
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
        delete *iter;
}

void ListExpression::Add(Expression *expression)
{
    if (expressions.empty())
        (SourceElement &)*this = *expression;
    expressions.push_back(expression);
}

bool ListExpression::Empty() const
{
    return expressions.empty();
}

void ListExpression::Parent(const Statement *statement)
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Parent(statement);
    }
}

TupleExpression::TupleExpression(const Token &token) :
    Expression(token)
{
}

TupleExpression::~TupleExpression()
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
        delete *iter;
}

void TupleExpression::Add(Expression *expression)
{
    if (expressions.empty())
        (SourceElement &)*this = *expression;
    expressions.push_back(expression);
}

void TupleExpression::Parent(const Statement *statement)
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Parent(statement);
    }
}

RangeExpression::RangeExpression
    (const Token &token,
     Expression *startExpression, Expression *endExpression,
     Expression *stepExpression) :
    Expression(token),
    startExpression(startExpression),
    endExpression(endExpression),
    stepExpression(stepExpression)
{
}

RangeExpression::~RangeExpression()
{
    delete startExpression;
    delete endExpression;
    delete stepExpression;
}

void RangeExpression::Parent(const Statement *statement)
{
    if (startExpression != 0)
        startExpression->Parent(statement);
    if (endExpression != 0)
        endExpression->Parent(statement);
    if (stepExpression != 0)
        stepExpression->Parent(statement);
}

ConstantExpression::ConstantExpression(const Token &token) :
    Expression(token)
{
    switch (token.type)
    {
        default:
            throw string("Invalid token");

        case TOKEN_NONE:
            type = Type::None;
            break;

        case TOKEN_ELLIPSIS:
            type = Type::Ellipsis;
            break;

        case TOKEN_FALSE:
            type = Type::Boolean;
            b = false;
            break;

        case TOKEN_TRUE:
            type = Type::Boolean;
            b = true;
            break;

        case TOKEN_INTEGER:
            type = Type::Integer;
            i = token.i;
            break;

        case TOKEN_FLOAT:
            type = Type::Float;
            f = token.f;
            break;

        case TOKEN_STRING:
            type = Type::String;
            s = token.s;
            break;
    }
}

ConstantExpression *ConstantExpression::FoldUnary
    (int operatorTokenType) const
{
    switch (operatorTokenType)
    {
        case TOKEN_NOT:
            return Not();
        case TOKEN_PLUS:
            return Plus();
        case TOKEN_MINUS:
            return Minus();
        case TOKEN_TILDE:
            return Invert();
    }

    throw string("Invalid unary operator");
}

ConstantExpression *ConstantExpression::FoldBinary
    (int operatorTokenType, const ConstantExpression *rightExpression) const
{
    switch (operatorTokenType)
    {
        case TOKEN_OR:
            return Or(rightExpression);
        // TODO: Add other operations.
        #if 0
        case TOKEN_AND:
            return And(rightExpression);
        case TOKEN_EQ:
            return Equal(rightExpression);
        case TOKEN_NE:
            return NotEqual(rightExpression);
        case TOKEN_LT:
            return Less(rightExpression);
        case TOKEN_LE:
            return LessOrEqual(rightExpression);
        case TOKEN_GT:
            return Greater(rightExpression);
        case TOKEN_GE:
            return GreaterOrEqual(rightExpression);
        case TOKEN_BAR:
            return BitOr(rightExpression);
        case TOKEN_CARET:
            return BitExclusiveOr(rightExpression);
        case TOKEN_AMPERSAND:
            return BitAnd(rightExpression);
        case TOKEN_LEFT_SHIFT:
            return LeftShift(rightExpression);
        case TOKEN_RIGHT_SHIFT:
            return RightShift(rightExpression);
        case TOKEN_ASTERISK:
            return Multiply(rightExpression);
        case TOKEN_SLASH:
            return Divide(rightExpression);
        case TOKEN_FLOOR_DIVIDE:
            return FloorDivide(rightExpression);
        case TOKEN_PERCENT:
            return Modulo(rightExpression);
        case TOKEN_POWER:
            return Power(rightExpression);
        #endif
    }

    throw string("Invalid binary operator");
}

ConstantExpression *ConstantExpression::FoldTernary
    (int operatorTokenType,
     const ConstantExpression *trueExpression,
     const ConstantExpression *falseExpression) const
{
    // TODO: Implement.
}

ConstantExpression *ConstantExpression::Not() const
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary not");

        case Type::None:
            return new ConstantExpression(Token(sourceLocation, TOKEN_TRUE));

        case Type::Ellipsis:
            return new ConstantExpression(Token(sourceLocation, TOKEN_FALSE));

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? TOKEN_FALSE : TOKEN_TRUE));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation,
                i != 0 ? TOKEN_FALSE : TOKEN_TRUE));

        case Type::Float:
            return new ConstantExpression(Token(sourceLocation,
                f != 0 ? TOKEN_FALSE : TOKEN_TRUE));
    }
}

ConstantExpression *ConstantExpression::Plus() const
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary +");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? 1 : 0, 0));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation, i, 0));

        case Type::Float:
            return new ConstantExpression(Token(sourceLocation, f));
    }
}

ConstantExpression *ConstantExpression::Minus() const
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary -");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? -1 : 0));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation, -i, 0));

        case Type::Float:
            return new ConstantExpression(Token(sourceLocation, -f));
    }
}

ConstantExpression *ConstantExpression::Invert() const
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary ~");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                ~(b ? 1 : 0), 0));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation, ~i, 0));
    }
}

ConstantExpression *ConstantExpression::Or
    (const ConstantExpression *right) const
{
    // TODO: Implement this and other operators.
}
