//
// Asp expression implementation.
//

#include "expression.hpp"
#include "asp.h"
#include "integer.h"
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
    type(Type::NonGroup),
    name(nameToken.s),
    valueExpression(valueExpression)
{
}

Argument::Argument(Expression *valueExpression, Type type) :
    NonTerminal((SourceElement &)*valueExpression),
    type(type),
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
        return nullptr;
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
    if (startExpression != nullptr)
        startExpression->Parent(statement);
    if (endExpression != nullptr)
        endExpression->Parent(statement);
    if (stepExpression != nullptr)
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
            type = token.negatedMinInteger ?
                Type::NegatedMinInteger : Type::Integer;
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
    if (constExpression == nullptr)
        return nullptr;

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
    typedef ConstantExpression::Type Type;

    auto *leftConstExpression = dynamic_cast<ConstantExpression *>
        (leftExpression);
    auto *rightConstExpression = dynamic_cast<ConstantExpression *>
        (rightExpression);
    if (leftConstExpression == nullptr && rightConstExpression == nullptr)
        return nullptr;

    switch (operatorTokenType)
    {
        case TOKEN_OR:
            return
                leftConstExpression == nullptr ? nullptr :
                FoldOr(leftConstExpression, rightExpression);

        case TOKEN_AND:
            return
                leftConstExpression == nullptr ? nullptr :
                FoldAnd(leftConstExpression, rightExpression);

        case TOKEN_EQ:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldEqual(leftConstExpression, rightConstExpression);

        case TOKEN_NE:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldNotEqual(leftConstExpression, rightConstExpression);

        case TOKEN_LT:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldLess(leftConstExpression, rightConstExpression);

        case TOKEN_LE:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldLessOrEqual(leftConstExpression, rightConstExpression);

        case TOKEN_GT:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldGreater(leftConstExpression, rightConstExpression);

        case TOKEN_GE:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldGreaterOrEqual(leftConstExpression, rightConstExpression);

        case TOKEN_IN:
        case TOKEN_NOT_IN:
        case TOKEN_IS:
        case TOKEN_IS_NOT:
            return nullptr;

        case TOKEN_BAR:
        case TOKEN_CARET:
        case TOKEN_AMPERSAND:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                FoldBitwiseOperation
                    (operatorTokenType,
                     leftConstExpression, rightConstExpression);

        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_ASTERISK:
        case TOKEN_SLASH:
        case TOKEN_FLOOR_DIVIDE:
        case TOKEN_PERCENT:
        case TOKEN_DOUBLE_ASTERISK:
            return
                leftConstExpression == nullptr ||
                rightConstExpression == nullptr ?
                nullptr :
                leftConstExpression->type == Type::String &&
                rightConstExpression->type == Type::String ?
                FoldStringConcatenationOperation
                    (operatorTokenType,
                     leftConstExpression, rightConstExpression) :
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
                conditionConstExpression == nullptr ? nullptr :
                FoldConditional
                    (conditionConstExpression,
                     trueExpression, falseExpression);
    }

    throw string("Invalid ternary operator");
}

bool ConstantExpression::IsTrue() const
{
    if (type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

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
            return f != 0.0;

        case Type::String:
            return !s.empty();
    }
}

bool ConstantExpression::IsString() const
{
    return type == Type::String;
}

bool ConstantExpression::IsEqual(const ConstantExpression &right) const
{
    if (type == Type::NegatedMinInteger ||
        right.type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

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
    if (type == Type::NegatedMinInteger ||
        right.type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    bool comparable =
        (type == Type::Boolean ||
         type == Type::Integer ||
         type == Type::Float) &&
        (right.type == Type::Boolean ||
         right.type == Type::Integer ||
         right.type == Type::Float) ||
        type == Type::String && right.type == Type::String;
    if (!comparable)
        throw string("Invalid operand types in comparison expression");
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
        throw string
            ("Internal error: Invalid type in numeric comparison expression");
    if (right.type == Type::Boolean)
        rightInt = right.b ? 1 : 0;
    else if (right.type == Type::Integer)
        rightInt = right.i;
    else
        throw string
            ("Internal error: Invalid type in numeric comparison expression");
    Type resultType = Type::Integer;
    double leftFloat = 0.0, rightFloat = 0.0;
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
        throw string
            ("Internal error: Invalid type in string comparison expression");

    return s == right.s ? 0 : s < right.s ? -1 : +1;
}

Expression *ConstantExpression::FoldNot()
{
    return new ConstantExpression(Token(sourceLocation,
        !IsTrue() ? TOKEN_TRUE : TOKEN_FALSE));
}

Expression *ConstantExpression::FoldPlus()
{
    if (type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    switch (type)
    {
        default:
            throw string("Invalid operand type in unary positive expression");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? 1 : 0, false));

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
            throw string
                ("Invalid operand type in unary negation expression");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                b ? -1 : 0, false));

        case Type::Integer:
        {
            int32_t intResult;
            AspIntegerResult result = AspNegateInteger(i, &intResult);
            if (result == AspIntegerResult_ArithmeticOverflow)
                throw string
                    ("Arithmetic overflow in unary negation expression");
            else if (result != AspIntegerResult_OK)
                throw string("Invalid unary negation expression");
            return new ConstantExpression(Token(sourceLocation,
                intResult, false));
        }

        case Type::NegatedMinInteger:
            return new ConstantExpression(Token(sourceLocation,
                INT32_MIN, false));

        case Type::Float:
            return new ConstantExpression(Token(sourceLocation, -f));
    }
}

Expression *ConstantExpression::FoldInvert()
{
    if (type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    switch (type)
    {
        default:
            throw string
                ("Invalid operand type in unary invert expression");

        case Type::Boolean:
            return new ConstantExpression(Token(sourceLocation,
                ~(b ? 1 : 0), false));

        case Type::Integer:
            return new ConstantExpression(Token(sourceLocation,
                ~i, false));
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

    if (leftExpression->type == Type::NegatedMinInteger ||
        rightExpression->type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    int32_t leftValue = 0, rightValue = 0;
    if (leftExpression->type == Type::Boolean)
        leftValue = leftExpression->b ? 1 : 0;
    else if (leftExpression->type == Type::Integer)
        leftValue = leftExpression->i;
    else
        throw string
            ("Invalid left operand type in binary bitwise expression");
    if (rightExpression->type == Type::Boolean)
        rightValue = rightExpression->b ? 1 : 0;
    else if (rightExpression->type == Type::Integer)
        rightValue = rightExpression->i;
    else
        throw string
            ("Invalid right operand type in binary bitwise expression");

    int32_t intResult = 0;
    switch (operatorTokenType)
    {
        default:
            return nullptr;

        case TOKEN_BAR:
        {
            AspIntegerResult result = AspBitwiseOrIntegers
                (leftValue, rightValue, &intResult);
            if (result != AspIntegerResult_OK)
                throw string("Invalid bitwise or expression");
            break;
        }

        case TOKEN_CARET:
        {
            AspIntegerResult result = AspBitwiseExclusiveOrIntegers
                (leftValue, rightValue, &intResult);
            if (result != AspIntegerResult_OK)
                throw string("Invalid bitwise or expression");
            break;
        }

        case TOKEN_AMPERSAND:
        {
            AspIntegerResult result = AspBitwiseAndIntegers
                (leftValue, rightValue, &intResult);
            if (result != AspIntegerResult_OK)
                throw string("Invalid bitwise or expression");
            break;
        }

        case TOKEN_LEFT_SHIFT:
        {
            AspIntegerResult result = AspLeftShiftInteger
                (leftValue, rightValue, &intResult);
            if (result == AspIntegerResult_ValueOutOfRange)
                throw string
                    ("Out of range value(s) in binary left shift expression");
            else if (result != AspIntegerResult_OK)
                throw string("Invalid left shift expression");
            break;
        }

        case TOKEN_RIGHT_SHIFT:
        {
            AspIntegerResult result = AspRightShiftInteger
                (leftValue, rightValue, &intResult);
            if (result == AspIntegerResult_ValueOutOfRange)
                throw string
                    ("Out of range value(s) in binary right shift expression");
            else if (result != AspIntegerResult_OK)
                throw string("Invalid right shift expression");
            break;
        }
    }

    return new ConstantExpression
        (Token(leftExpression->sourceLocation, intResult, false));
}

Expression *FoldStringConcatenationOperation
    (int operatorTokenType,
     ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    typedef ConstantExpression::Type Type;

    if (leftExpression->type != Type::String ||
        rightExpression->type != Type::String)
        return nullptr;

    if (rightExpression->s.empty())
        return leftExpression;
    if (leftExpression->s.empty())
        return rightExpression;

    return new ConstantExpression(Token(leftExpression->sourceLocation,
        leftExpression->s + rightExpression->s));
}

Expression *FoldArithmeticOperation
    (int operatorTokenType,
     ConstantExpression *leftExpression, ConstantExpression *rightExpression)
{
    typedef ConstantExpression::Type Type;

    if (leftExpression->type == Type::NegatedMinInteger ||
        rightExpression->type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    // Fold only numeric operands. Do not fold, for example, string
    // multiplication.
    int32_t leftInt = 0, rightInt = 0;
    if (leftExpression->type == Type::Boolean)
        leftInt = leftExpression->b ? 1 : 0;
    else if (leftExpression->type == Type::Integer)
        leftInt = leftExpression->i;
    else if (leftExpression->type != Type::Float)
        return nullptr;
    uint32_t leftUnsigned = *reinterpret_cast<uint32_t *>(&leftInt);
    if (rightExpression->type == Type::Boolean)
        rightInt = rightExpression->b ? 1 : 0;
    else if (rightExpression->type == Type::Integer)
        rightInt = rightExpression->i;
    else if (rightExpression->type != Type::Float)
        return nullptr;
    uint32_t rightUnsigned = *reinterpret_cast<uint32_t *>(&rightInt);
    Type resultType = Type::Integer;
    double leftFloat = 0.0, rightFloat = 0.0;
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
    double floatResult = 0.0;
    string desc;
    if (resultType == Type::Integer)
    {
        AspIntegerResult integerResult = AspIntegerResult_OK;
        switch (operatorTokenType)
        {
            default:
                return nullptr;

            case TOKEN_PLUS:
                desc = "addition";
                integerResult = AspAddIntegers
                    (leftInt, rightInt, &intResult);
                break;

            case TOKEN_MINUS:
                desc = "subtraction";
                integerResult = AspSubtractIntegers
                    (leftInt, rightInt, &intResult);
                break;

            case TOKEN_ASTERISK:
                desc = "multiplication";
                integerResult = AspMultiplyIntegers
                    (leftInt, rightInt, &intResult);
                break;

            case TOKEN_SLASH:
                if (rightInt == 0)
                    throw string("Divide by zero in division expression");
                resultType = Type::Float;
                floatResult =
                    static_cast<double>(leftInt) /
                    static_cast<double>(rightInt);
                break;

            case TOKEN_FLOOR_DIVIDE:
                desc = "division";
                integerResult = AspDivideIntegers
                    (leftInt, rightInt, &intResult);
                break;

            case TOKEN_PERCENT:
                desc = "modulo";
                integerResult = AspModuloIntegers
                    (leftInt, rightInt, &intResult);
                break;

            case TOKEN_DOUBLE_ASTERISK:
                resultType = Type::Float;
                floatResult = pow
                    (static_cast<double>(leftInt),
                     static_cast<double>(rightInt));
                break;
        }

        desc = "binary " + desc + " expression";
        switch (integerResult)
        {
            default:
                throw string("Invalid " + desc);
            case AspIntegerResult_OK:
                break;
            case AspIntegerResult_ValueOutOfRange:
                throw string("Out of range value(s) in " + desc);
            case AspIntegerResult_DivideByZero:
                throw string("Divide by zero in " + desc);
            case AspIntegerResult_ArithmeticOverflow:
                throw string("Arithmetic overflow in " + desc);
        }
    }
    else if (resultType == Type::Float)
    {
        switch (operatorTokenType)
        {
            default:
                return nullptr;

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
                if (rightFloat == 0.0)
                    throw string("Divide by zero in division expression");
                floatResult = leftFloat / rightFloat;
                break;

            case TOKEN_FLOOR_DIVIDE:
                if (rightFloat == 0.0)
                    throw string("Divide by zero in division expression");
                floatResult = floor(leftFloat / rightFloat);
                break;

            case TOKEN_PERCENT:
                if (rightFloat == 0.0)
                    throw string("Divide by zero in division expression");
                floatResult = static_cast<double>
                    (leftFloat - floor(leftFloat / rightFloat) * rightFloat);
                break;

            case TOKEN_DOUBLE_ASTERISK:
                floatResult = pow(leftFloat, rightFloat);
                break;
        }
    }

    return resultType == Type::Integer ?
        new ConstantExpression
            (Token(leftExpression->sourceLocation, intResult, false)) :
        new ConstantExpression
            (Token(leftExpression->sourceLocation, floatResult));
}

Expression *FoldConditional
    (ConstantExpression *conditionExpression,
     Expression *leftExpression, Expression *rightExpression)
{
    typedef ConstantExpression::Type Type;

    auto *leftConstExpression = dynamic_cast<ConstantExpression *>
        (leftExpression);
    auto *rightConstExpression = dynamic_cast<ConstantExpression *>
        (rightExpression);
    if (leftConstExpression != nullptr &&
        leftConstExpression->type == Type::NegatedMinInteger ||
        rightConstExpression != nullptr &&
        rightConstExpression->type == Type::NegatedMinInteger)
        throw string("Integer constant out of range");

    return conditionExpression->IsTrue() ? leftExpression : rightExpression;
}
