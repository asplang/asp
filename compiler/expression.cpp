//
// Asp expression implementation.
//

#include "expression.hpp"
#include "asp.h"
#include <cmath>

using namespace std;

Expression::Expression(const SourceElement &sourceElement) :
    NonTerminal(sourceElement)
{
}

void Expression::Enclose()
{
    enclosed = true;
}

bool Expression::IsEnclosed() const
{
    return enclosed;
}

void Expression::Parent(const Statement *statement)
{
    parentStatement = statement;
}

const Statement *Expression::Parent() const
{
    return parentStatement;
}

ConditionalExpression::ConditionalExpression
    (const Token &operatorToken, Expression *conditionExpression,
     Expression *trueExpression, Expression *falseExpression) :
    Expression(operatorToken),
    operatorTokenType(operatorToken.type),
    conditionExpression(conditionExpression),
    trueExpression(trueExpression),
    falseExpression(falseExpression)
{
}

ConditionalExpression::~ConditionalExpression()
{
    delete conditionExpression;
    delete trueExpression;
    delete falseExpression;
}

void ConditionalExpression::Parent(const Statement *statement)
{
    conditionExpression->Parent(statement);
    trueExpression->Parent(statement);
    falseExpression->Parent(statement);
}

ShortCircuitLogicalExpression::ShortCircuitLogicalExpression
    (const Token &operatorToken,
     Expression *leftExpression, Expression *rightExpression) :
    Expression(operatorToken),
    operatorTokenType(operatorToken.type)
{
    expressions.push_back(leftExpression);
    expressions.push_back(rightExpression);
}

ShortCircuitLogicalExpression::~ShortCircuitLogicalExpression()
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
        delete *iter;
}

void ShortCircuitLogicalExpression::Add(Expression *expression)
{
    expressions.push_back(expression);
}

void ShortCircuitLogicalExpression::Parent(const Statement *statement)
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Parent(statement);
    }
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

TargetExpression::TargetExpression()
{
}

TargetExpression::TargetExpression(const Token &token) :
    Expression(token)
{
    if (token.type == TOKEN_NAME)
        name = token.s;
}

TargetExpression::~TargetExpression()
{
    for (auto iter = targetExpressions.begin();
         iter != targetExpressions.end(); iter++)
        delete *iter;
}

bool TargetExpression::IsTuple() const
{
    return name.empty();
}

void TargetExpression::Add(TargetExpression *targetExpression)
{
    if (IsEnclosed())
        throw string("Internal error: Cannot add to a enclosed target tuple");
    if (targetExpressions.empty())
        (SourceElement &)*this = *targetExpression;
    targetExpressions.push_back(targetExpression);
}

void TargetExpression::Parent(const Statement *statement)
{
    parentStatement = statement;
}

Argument::Argument
    (const Token &nameToken, Expression *valueExpression) :
    NonTerminal(nameToken),
    name(nameToken.s),
    valueExpression(valueExpression),
    isGroup(false)
{
}

Argument::Argument(Expression *valueExpression, bool isGroup) :
    NonTerminal((SourceElement &)*valueExpression),
    valueExpression(valueExpression),
    isGroup(isGroup)
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
    Expression((SourceElement &)*functionExpression),
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
    Expression((SourceElement &)*indexExpression),
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
    Expression(nameToken),
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

SymbolExpression::SymbolExpression
    (const Token &operatorToken, const Token &nameToken) :
    Expression(operatorToken),
    name(nameToken.s)
{
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

SetExpression::SetExpression(const Token &token) :
    Expression(token)
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

void SetExpression::Parent(const Statement *statement)
{
    for (auto iter = expressions.begin(); iter != expressions.end(); iter++)
    {
        auto expression = *iter;
        expression->Parent(statement);
    }
}

ListExpression::ListExpression(const Token &token) :
    Expression(token)
{
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

bool ListExpression::IsEmpty() const
{
    return expressions.empty();
}

Expression *ListExpression::PopFront()
{
    if (expressions.empty())
        return 0;
    else
    {
        auto result = expressions.front();
        expressions.pop_front();
        return result;
    }
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

Expression *FoldUnaryExpression
    (int operatorTokenType, Expression *expression)
{
    auto *constExpression = dynamic_cast<ConstantExpression *>(expression);
    if (constExpression == 0)
        return 0;

    switch (operatorTokenType)
    {
        case TOKEN_NOT:
            return constExpression->FoldNot();

        case TOKEN_PLUS:
            return constExpression->FoldPlus();

        case TOKEN_MINUS:
            return constExpression->FoldMinus();

        case TOKEN_TILDE:
            return constExpression->FoldInvert();
    }

    throw string("Invalid unary operator");
}

Expression *FoldBinaryExpression
    (int operatorTokenType,
     Expression *leftExpression, Expression *rightExpression)
{
    auto *leftConstExpression = dynamic_cast<ConstantExpression *>
        (leftExpression);
    auto *rightConstExpression = dynamic_cast<ConstantExpression *>
        (rightExpression);
    if (leftConstExpression == 0 && rightConstExpression == 0)
        return 0;

    switch (operatorTokenType)
    {
        case TOKEN_OR:
            return
                leftConstExpression == 0 ? 0 :
                FoldOr(leftConstExpression, rightExpression);

        case TOKEN_AND:
            return
                leftConstExpression == 0 ? 0 :
                FoldAnd(leftConstExpression, rightExpression);

        case TOKEN_EQ:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldEqual(leftConstExpression, rightConstExpression);

        case TOKEN_NE:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldNotEqual(leftConstExpression, rightConstExpression);

        case TOKEN_LT:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldLess(leftConstExpression, rightConstExpression);

        case TOKEN_LE:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldLessOrEqual(leftConstExpression, rightConstExpression);

        case TOKEN_GT:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldGreater(leftConstExpression, rightConstExpression);

        case TOKEN_GE:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldGreaterOrEqual(leftConstExpression, rightConstExpression);

        case TOKEN_IN:
        case TOKEN_NOT_IN:
        case TOKEN_IS:
        case TOKEN_IS_NOT:
            return 0;

        case TOKEN_BAR:
        case TOKEN_CARET:
        case TOKEN_AMPERSAND:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldBitwiseOperation
                    (operatorTokenType,
                     leftConstExpression, rightConstExpression);

        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_ASTERISK:
        case TOKEN_SLASH:
        case TOKEN_FLOOR_DIVIDE:
        case TOKEN_PERCENT:
        case TOKEN_POWER:
            return
                leftConstExpression == 0 || rightConstExpression == 0 ? 0 :
                FoldArithmeticOperation
                    (operatorTokenType,
                     leftConstExpression, rightConstExpression);
    }

    throw string("Invalid binary operator");
}

Expression *FoldTernaryExpression
    (int operatorTokenType,
     Expression *conditionExpression,
     Expression *trueExpression, Expression *falseExpression)
{
    auto *conditionConstExpression = dynamic_cast<ConstantExpression *>
        (conditionExpression);

    switch (operatorTokenType)
    {
        case TOKEN_IF:
            return
                conditionConstExpression == 0 ? 0 :
                FoldConditional
                    (conditionConstExpression,
                     trueExpression, falseExpression);
    }

    throw string("Invalid ternary operator");
}

bool ConstantExpression::IsTrue() const
{
    switch (type)
    {
        default:
            return true;

        case Type::None:
            return false;

        case Type::Boolean:
            return b;

        case Type::Integer:
            return i != 0;

        case Type::Float:
            return f != 0;

        case Type::String:
            return !s.empty();
    }
}

bool ConstantExpression::IsEqual(const ConstantExpression &right) const
{
    if (type != right.type)
        return false;
    switch (type)
    {
        default:
            return false;

        case Type::None:
        case Type::Ellipsis:
            return true;

        case Type::Boolean:
            return b == right.b;

        case Type::Integer:
            return i == right.i;

        case Type::Float:
            return f == right.f;

        case Type::String:
            return s == right.s;
    }
}

int ConstantExpression::Compare(const ConstantExpression &right) const
{
    bool comparable =
        (type == Type::Boolean ||
         type == Type::Integer ||
         type == Type::Float) &&
        (right.type == Type::Boolean ||
         right.type == Type::Integer ||
         right.type == Type::Float) ||
        type == Type::String && right.type == Type::String;
    if (!comparable)
        throw string("Bad operand types for comparison operation");
    return
        type == Type::String ?
        StringCompare(right) : NumericCompare(right);
}

int ConstantExpression::NumericCompare(const ConstantExpression &right) const
{
    int32_t leftInt = 0, rightInt = 0;
    if (type == Type::Boolean)
        leftInt = b ? 1 : 0;
    else if (type == Type::Integer)
        leftInt = i;
    else
        throw string("Internal error: Invalid type for numeric comparison");
    if (right.type == Type::Boolean)
        rightInt = right.b ? 1 : 0;
    else if (right.type == Type::Integer)
        rightInt = right.i;
    else
        throw string("Internal error: Invalid type for numeric comparison");
    Type resultType = Type::Integer;
    double leftFloat = 0, rightFloat = 0;
    if (type == Type::Float || right.type == Type::Float)
    {
        resultType = Type::Float;
        if (type != Type::Float)
            leftFloat = static_cast<double>(leftInt);
        else
            leftFloat = f;
        if (right.type != Type::Float)
            rightFloat = static_cast<double>(rightInt);
        else
            rightFloat = right.f;
    }

    return
        resultType == Type::Integer ?
            leftInt == rightInt ? 0 : leftInt < rightInt ? -1 : +1 :
            leftFloat == rightFloat ? 0 : leftFloat < rightFloat ? -1 : +1;
}

int ConstantExpression::StringCompare(const ConstantExpression &right) const
{
    if (type != Type::String || right.type != Type::String)
        throw string("Internal error: Invalid type for string comparison");

    return s == right.s ? 0 : s < right.s ? -1 : +1;
}

Expression *ConstantExpression::FoldNot()
{
    return new ConstantExpression(Token(sourceLocation,
        !IsTrue() ? TOKEN_TRUE : TOKEN_FALSE));
}

Expression *ConstantExpression::FoldPlus()
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary +");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? 1 : 0, 0));

        case Type::Integer:
        case Type::Float:
            return this;
    }
}

Expression *ConstantExpression::FoldMinus()
{
    switch (type)
    {
        default:
            throw string("Bad operand type for unary -");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? -1 : 0, 0));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation, -i, 0));

        case Type::Float:
            return new ConstantExpression(Token(sourceLocation, -f));
    }
}

Expression *ConstantExpression::FoldInvert()
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

Expression *FoldOr
    (ConstantExpression *leftExpression, Expression *rightExpression)
{
    return leftExpression->IsTrue() ? leftExpression : rightExpression;
}

Expression *FoldAnd
    (ConstantExpression *leftExpression, Expression *rightExpression)
{
    return !leftExpression->IsTrue() ? leftExpression : rightExpression;
}

Expression *FoldEqual
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->IsEqual(*rightExpression) ? TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldNotEqual
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        !leftExpression->IsEqual(*rightExpression) ? TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldLess
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->Compare(*rightExpression) < 0 ?
        TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldLessOrEqual
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->Compare(*rightExpression) <= 0 ?
        TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldGreater
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->Compare(*rightExpression) > 0 ?
        TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldGreaterOrEqual
    (ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->Compare(*rightExpression) >= 0 ?
        TOKEN_TRUE : TOKEN_FALSE));
}

Expression *FoldBitwiseOperation
    (int operatorTokenType,
     ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    typedef ConstantExpression::Type Type;

    int32_t leftValue = 0, rightValue = 0;
    uint32_t leftBits = 0, rightBits = 0;
    if (leftExpression->type == Type::Boolean)
        leftValue = leftExpression->b ? 1 : 0;
    else if (leftExpression->type == Type::Integer)
        leftValue = leftExpression->i;
    else
        throw string("Bad left operand type for binary bitwise operation");
    leftBits = *reinterpret_cast<uint32_t *>(&leftValue);
    if (rightExpression->type == Type::Boolean)
        rightValue = rightExpression->b ? 1 : 0;
    else if (rightExpression->type == Type::Integer)
        rightValue = rightExpression->i;
    else
        throw string("Bad right operand type for binary bitwise operation");
    rightBits = *reinterpret_cast<uint32_t *>(&rightValue);

    uint32_t resultBits = 0;
    switch (operatorTokenType)
    {
        default:
            return 0;

            case TOKEN_BAR:
                resultBits = leftBits | rightBits;
                break;

            case TOKEN_CARET:
                resultBits = leftBits ^ rightBits;
                break;

            case TOKEN_AMPERSAND:
                resultBits = leftBits & rightBits;
                break;

            case TOKEN_LEFT_SHIFT:
                if (rightValue < 0)
                    throw string("Negative shift count not permitted");
                resultBits = leftBits << rightValue;
                break;

            case TOKEN_RIGHT_SHIFT:
            {
                if (rightValue < 0)
                    throw string("Negative shift count not permitted");

                // Perform sign extension.
                int32_t resultValue = leftValue >> rightValue;
                resultBits = *reinterpret_cast<uint32_t *>(&resultValue);
                break;
            }
    }

    int32_t resultValue = *reinterpret_cast<int32_t *>(&resultBits);
    return new ConstantExpression
        (Token(leftExpression->sourceLocation, resultValue, 0));
}

Expression *FoldArithmeticOperation
    (int operatorTokenType,
     ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    typedef ConstantExpression::Type Type;

    // Fold only numeric operands. Do not fold, for example, string
    // multiplication.
    int32_t leftInt = 0, rightInt = 0;
    uint32_t leftBits = 0, rightBits = 0;
    if (leftExpression->type == Type::Boolean)
        leftInt = leftExpression->b ? 1 : 0;
    else if (leftExpression->type == Type::Integer)
        leftInt = leftExpression->i;
    else if (leftExpression->type != Type::Float)
        return 0;
    if (rightExpression->type == Type::Boolean)
        rightInt = rightExpression->b ? 1 : 0;
    else if (rightExpression->type == Type::Integer)
        rightInt = rightExpression->i;
    else if (rightExpression->type != Type::Float)
        return 0;
    Type resultType = Type::Integer;
    double leftFloat = 0, rightFloat = 0;
    if (leftExpression->type == Type::Float ||
        rightExpression->type == Type::Float)
    {
        resultType = Type::Float;
        if (leftExpression->type != Type::Float)
            leftFloat = static_cast<double>(leftInt);
        else
            leftFloat = leftExpression->f;
        if (rightExpression->type != Type::Float)
            rightFloat = static_cast<double>(rightInt);
        else
            rightFloat = rightExpression->f;
    }

    int intResult = 0;
    double floatResult = 0;
    if (resultType == Type::Integer)
    {
        switch (operatorTokenType)
        {
            default:
                return 0;

            case TOKEN_PLUS:
                intResult = leftInt + rightInt;
                break;

            case TOKEN_MINUS:
                intResult = leftInt - rightInt;
                break;

            case TOKEN_ASTERISK:
                intResult = leftInt * rightInt;
                break;

            case TOKEN_SLASH:
                if (rightInt == 0)
                    throw string("Divide by zero");
                resultType = Type::Float;
                floatResult =
                    static_cast<double>(leftInt) /
                    static_cast<double>(rightInt);
                break;

            case TOKEN_FLOOR_DIVIDE:
                if (rightInt == 0)
                    throw string("Divide by zero");
                intResult = leftInt / rightInt;
                if (leftInt < 0 != rightInt < 0 &&
                    leftInt != intResult * rightInt)
                     intResult--;
                break;

            case TOKEN_PERCENT:
            {
                if (rightInt == 0)
                    throw string("Divide by zero");

                /* Compute using the quotient rounded toward negative
                   infinity. */
                int32_t signedLeft = leftInt < 0 ? -leftInt : leftInt;
                int32_t signedRight = rightInt < 0 ? -rightInt : rightInt;
                int32_t quotient = signedLeft / signedRight;
                if (leftInt < 0 != rightInt < 0)
                {
                    quotient = -quotient;
                    if (signedLeft % signedRight != 0)
                        quotient--;
                }
                intResult = leftInt - quotient * rightInt;

                break;
            }

            case TOKEN_POWER:
                resultType = Type::Float;
                floatResult = pow
                    (static_cast<double>(leftInt),
                     static_cast<double>(rightInt));
                break;
        }
    }
    else if (resultType == Type::Float)
    {
        switch (operatorTokenType)
        {
            default:
                return 0;

            case TOKEN_PLUS:
                floatResult = leftFloat + rightFloat;
                break;

            case TOKEN_MINUS:
                floatResult = leftFloat - rightFloat;
                break;

            case TOKEN_ASTERISK:
                floatResult = leftFloat * rightFloat;
                break;

            case TOKEN_SLASH:
                if (rightFloat == 0)
                    throw string("Divide by zero");
                floatResult = leftFloat / rightFloat;
                break;

            case TOKEN_FLOOR_DIVIDE:
                if (rightFloat == 0)
                    throw string("Divide by zero");
                floatResult = floor(leftFloat / rightFloat);
                break;

            case TOKEN_PERCENT:
                if (rightFloat == 0)
                    throw string("Divide by zero");
                floatResult = static_cast<double>
                    (leftFloat - floor(leftFloat / rightFloat) * rightFloat);
                break;

            case TOKEN_POWER:
                floatResult = pow(leftFloat, rightFloat);
                break;
        }
    }

    return resultType == Type::Integer ?
        new ConstantExpression
            (Token(leftExpression->sourceLocation, intResult, 0)) :
        new ConstantExpression
            (Token(leftExpression->sourceLocation, floatResult));
}

Expression *FoldConditional
    (ConstantExpression *conditionExpression,
     Expression *leftExpression, Expression *rightExpression)
{
    return conditionExpression->IsTrue() ? leftExpression : rightExpression;
}
