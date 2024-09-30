//
// Asp statement implementation.
//

#include "statement.hpp"

using namespace std;

Statement::Statement(const SourceElement &sourceElement) :
    NonTerminal(sourceElement)
{
}

void Statement::Parent(const Block *block)
{
    parentBlock = block;
}

const Block *Statement::Parent() const
{
    return parentBlock;
}

unsigned Statement::StackUsage() const
{
    return 0;
}

const LoopStatement *Statement::ParentLoop() const
{
    // Search for parent loop statement, ending the search if we reach a
    // function definition or the top level first.
    const LoopStatement *loopStatement = nullptr;
    for (auto parentStatement = Parent()->Parent();
         parentStatement != nullptr && loopStatement == nullptr;
         loopStatement = dynamic_cast<const LoopStatement *>(parentStatement),
         parentStatement = parentStatement->Parent()->Parent())
    {
        auto defStatement =
            dynamic_cast<const DefStatement *>(parentStatement);
        if (defStatement != nullptr)
            break;
    }
    return loopStatement;
}

const DefStatement *Statement::ParentDef() const
{
    // Search for parent function definition statement, ending the search
    // if we reach the top level first.
    const DefStatement *defStatement = nullptr;
    for (auto parentStatement = Parent()->Parent();
         parentStatement != nullptr && defStatement == nullptr;
         defStatement = dynamic_cast<const DefStatement *>(parentStatement),
         parentStatement = parentStatement->Parent()->Parent()) ;
    return defStatement;
}

Block::~Block()
{
    for (auto &statement: statements)
        delete statement;
}

void Block::Add(Statement *statement)
{
    if (statements.empty())
        (SourceElement &)*this = *statement;
    statement->Parent(this);
    statements.push_back(statement);
}

void Block::Parent(const Statement *statement)
{
    parentStatement = statement;
}

const Statement *Block::Parent() const
{
    return parentStatement;
}

const Statement *Block::FinalStatement() const
{
    return statements.empty() ? nullptr : statements.back();
}

ExpressionStatement::ExpressionStatement(Expression *expression) :
    Statement(*expression),
    expression(expression)
{
    expression->Parent(this);
}

ExpressionStatement::~ExpressionStatement()
{
    delete expression;
}

AssignmentStatement::AssignmentStatement
    (const Token &assignmentToken, Expression *targetExpression,
     AssignmentStatement *valueAssignmentStatement) :
    Statement(assignmentToken),
    assignmentTokenType(assignmentToken.type),
    targetExpression(targetExpression),
    valueAssignmentStatement(valueAssignmentStatement)
{
    targetExpression->Parent(this);
}

AssignmentStatement::AssignmentStatement
    (const Token &assignmentToken, Expression *targetExpression,
     Expression *valueExpression) :
    Statement(assignmentToken),
    assignmentTokenType(assignmentToken.type),
    targetExpression(targetExpression),
    valueExpression(valueExpression)
{
    targetExpression->Parent(this);
    valueExpression->Parent(this);
}

AssignmentStatement::~AssignmentStatement()
{
    delete targetExpression;
    delete valueExpression;
    delete valueAssignmentStatement;
}

void AssignmentStatement::Parent(const Block *block)
{
    if (valueAssignmentStatement)
        valueAssignmentStatement->Parent(block);
    Statement::Parent(block);
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     InsertionStatement *containerInsertionStatement,
     Expression *itemExpression) :
    Statement(insertionToken),
    containerInsertionStatement(containerInsertionStatement),
    itemExpression(itemExpression)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     InsertionStatement *containerInsertionStatement,
     KeyValuePair *keyValuePair) :
    Statement(insertionToken),
    containerInsertionStatement(containerInsertionStatement),
    keyValuePair(keyValuePair)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     Expression *containerExpression, Expression *itemExpression) :
    Statement(insertionToken),
    containerExpression(containerExpression),
    itemExpression(itemExpression)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     Expression *containerExpression, KeyValuePair *keyValuePair) :
    Statement(insertionToken),
    containerExpression(containerExpression),
    keyValuePair(keyValuePair)
{
}

InsertionStatement::~InsertionStatement()
{
    delete containerInsertionStatement;
    delete containerExpression;
    delete itemExpression;
    delete keyValuePair;
}

BreakStatement::BreakStatement(const Token &token) :
    Statement(token)
{
}

ContinueStatement::ContinueStatement(const Token &token) :
    Statement(token)
{
}

PassStatement::PassStatement(const Token &token) :
    Statement(token)
{
}

ImportName::ImportName(const Token &nameToken) :
    NonTerminal(nameToken),
    name(nameToken.s)
{
}

ImportName::ImportName(const Token &nameToken, const Token &asNameToken) :
    NonTerminal(nameToken),
    name(nameToken.s),
    asName(asNameToken.s)
{
}

void ImportNameList::Add(ImportName *name)
{
    if (names.empty())
        (SourceElement &)*this = *name;
    names.push_back(name);
}

ImportNameList::~ImportNameList()
{
    for (auto &name: names)
        delete name;
}

ImportStatement::ImportStatement
    (ImportNameList *moduleNameList, ImportNameList *memberNameList) :
    Statement(*moduleNameList),
    moduleNameList(moduleNameList),
    memberNameList(memberNameList)
{
}

ImportStatement::~ImportStatement()
{
    delete moduleNameList;
    delete memberNameList;
}

void VariableList::Add(const Token &nameToken)
{
    if (names.empty())
        (SourceElement &)*this = nameToken;
    names.push_back(nameToken.s);
}

void VariableList::Parent(const Statement *statement)
{
    parentStatement = statement;
}

GlobalStatement::GlobalStatement(VariableList *variableList) :
    Statement(*variableList),
    variableList(variableList)
{
}

GlobalStatement::~GlobalStatement()
{
    delete variableList;
}

LocalStatement::LocalStatement(VariableList *variableList) :
    Statement(*variableList),
    variableList(variableList)
{
}

LocalStatement::~LocalStatement()
{
    delete variableList;
}

DelStatement::DelStatement(Expression *expression) :
    ExpressionStatement(expression)
{
}

ReturnStatement::ReturnStatement
    (const Token &keywordToken, Expression *expression) :
    Statement(keywordToken),
    expression(expression)
{
    if (expression)
        expression->Parent(this);
}

ReturnStatement::~ReturnStatement()
{
    delete expression;
}

AssertStatement::AssertStatement(Expression *expression) :
    ExpressionStatement(expression)
{
}

IfStatement::IfStatement
    (Expression *conditionExpression, Block *trueBlock,
     IfStatement *elsePart) :
    Statement(*conditionExpression),
    conditionExpression(conditionExpression),
    trueBlock(trueBlock),
    elsePart(elsePart)
{
    conditionExpression->Parent(this);
    trueBlock->Parent(this);
}

IfStatement::IfStatement
    (Expression *conditionExpression, Block *trueBlock, Block *falseBlock) :
    Statement(*conditionExpression),
    conditionExpression(conditionExpression),
    trueBlock(trueBlock),
    falseBlock(falseBlock)
{
    conditionExpression->Parent(this);
    trueBlock->Parent(this);
    falseBlock->Parent(this);
}

IfStatement::IfStatement
    (Expression *conditionExpression, Block *trueBlock) :
    Statement(*conditionExpression),
    conditionExpression(conditionExpression),
    trueBlock(trueBlock)
{
    conditionExpression->Parent(this);
    trueBlock->Parent(this);
}

IfStatement::~IfStatement()
{
    delete conditionExpression;
    delete trueBlock;
    delete falseBlock;
    delete elsePart;
}

void IfStatement::Parent(const Block *block)
{
    if (elsePart)
        elsePart->Parent(block);
    Statement::Parent(block);
}


LoopStatement::LoopStatement(const SourceElement &sourceElement) :
    Statement(sourceElement)
{
}

Executable::Location LoopStatement::ContinueLocation() const
{
    return continueLocation;
}

Executable::Location LoopStatement::EndLocation() const
{
    return endLocation;
}

WhileStatement::WhileStatement
    (Expression *conditionExpression, Block *trueBlock, Block *falseBlock) :
    LoopStatement(*conditionExpression),
    conditionExpression(conditionExpression),
    trueBlock(trueBlock),
    falseBlock(falseBlock)
{
    conditionExpression->Parent(this);
    trueBlock->Parent(this);
    if (falseBlock)
        falseBlock->Parent(this);
}

WhileStatement::~WhileStatement()
{
    delete conditionExpression;
    delete trueBlock;
    delete falseBlock;
}

ForStatement::ForStatement
    (TargetExpression *targetExpression, Expression *iterableExpression,
     Block *trueBlock, Block *falseBlock) :
    LoopStatement(*targetExpression),
    targetExpression(targetExpression),
    iterableExpression(iterableExpression),
    trueBlock(trueBlock),
    falseBlock(falseBlock)
{
    targetExpression->Parent(this);
    iterableExpression->Parent(this);
    trueBlock->Parent(this);
    if (falseBlock)
        falseBlock->Parent(this);
}

ForStatement::~ForStatement()
{
    delete targetExpression;
    delete iterableExpression;
    delete trueBlock;
    delete falseBlock;
}

unsigned ForStatement::StackUsage() const
{
    return 1;
}

Parameter::Parameter
    (const Token &nameToken, Type type,
     Expression *defaultExpression) :
    NonTerminal(nameToken),
    name(nameToken.s),
    type(type),
    defaultExpression(defaultExpression)
{
    if (defaultExpression != nullptr &&
        (type == Type::TupleGroup || type == Type::DictionaryGroup))
        throw string("Group parameter cannot have a default value");
}

Parameter::~Parameter()
{
    delete defaultExpression;
}

void Parameter::Parent(const Statement *statement) const
{
    if (defaultExpression)
        defaultExpression->Parent(statement);
}

void ParameterList::Add(Parameter *parameter)
{
    if (parameters.empty())
        (SourceElement &)*this = *parameter;
    parameters.push_back(parameter);
}

ParameterList::~ParameterList()
{
    for (auto &parameter: parameters)
        delete parameter;
}

void ParameterList::Parent(const Statement *statement) const
{
    for (auto &parameter: parameters)
        parameter->Parent(statement);
}

DefStatement::DefStatement
    (const Token &nameToken, ParameterList *parameterList, Block *block) :
    Statement(nameToken),
    name(nameToken.s),
    parameterList(parameterList),
    block(block)
{
    parameterList->Parent(this);
    block->Parent(this);
}

DefStatement::~DefStatement()
{
    delete parameterList;
    delete block;
}
