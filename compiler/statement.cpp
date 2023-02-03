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

Block::Block()
{
}

Block::~Block()
{
    for (auto iter = statements.begin(); iter != statements.end(); iter++)
        delete *iter;
}

void Block::Add(Statement *statement)
{
    if (statements.empty())
        (SourceElement &)*this = *statement;
    statement->Parent(this);
    statements.push_back(statement);
}

void Block::Parent(Statement *statement)
{
    parentStatement = statement;
}

const Statement *Block::Parent() const
{
    return parentStatement;
}

const Statement *Block::FinalStatement() const
{
    return statements.empty() ? 0 : statements.back();
}

const LoopStatement *Statement::ParentLoop() const
{
    // Search for parent loop statement, ending the search if we reach a
    // function definition or the top level first.
    const LoopStatement *loopStatement = 0;
    for (auto parentStatement = Parent()->Parent();
         parentStatement != 0 && loopStatement == 0;
         loopStatement = dynamic_cast<const LoopStatement *>(parentStatement),
         parentStatement = parentStatement->Parent()->Parent())
    {
        auto defStatement =
            dynamic_cast<const DefStatement *>(parentStatement);
        if (defStatement != 0)
            break;
    }
    return loopStatement;
}

const DefStatement *Statement::ParentDef() const
{
    // Search for parent function definition statement, ending the search
    // if we reach a the top level first.
    const DefStatement *defStatement = 0;
    for (auto parentStatement = Parent()->Parent();
         parentStatement != 0 && defStatement == 0;
         defStatement = dynamic_cast<const DefStatement *>(parentStatement),
         parentStatement = parentStatement->Parent()->Parent()) ;
    return defStatement;
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
    valueExpression(0),
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
    valueExpression(valueExpression),
    valueAssignmentStatement(0)
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
    containerExpression(0),
    itemExpression(itemExpression),
    keyValuePair(0)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     InsertionStatement *containerInsertionStatement,
     KeyValuePair *keyValuePair) :
    Statement(insertionToken),
    containerInsertionStatement(containerInsertionStatement),
    containerExpression(0),
    itemExpression(0),
    keyValuePair(keyValuePair)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     Expression *containerExpression, Expression *itemExpression) :
    Statement(insertionToken),
    containerInsertionStatement(0),
    containerExpression(containerExpression),
    itemExpression(itemExpression),
    keyValuePair(0)
{
}

InsertionStatement::InsertionStatement
    (const Token &insertionToken,
     Expression *containerExpression, KeyValuePair *keyValuePair) :
    Statement(insertionToken),
    containerInsertionStatement(0),
    containerExpression(containerExpression),
    itemExpression(0),
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

ImportNameList::ImportNameList()
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
    for (auto iter = names.begin(); iter != names.end(); iter++)
        delete *iter;
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

VariableList::VariableList()
{
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
    falseBlock(0),
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
    falseBlock(falseBlock),
    elsePart(0)
{
    conditionExpression->Parent(this);
    trueBlock->Parent(this);
    falseBlock->Parent(this);
}

IfStatement::IfStatement
    (Expression *conditionExpression, Block *trueBlock) :
    Statement(*conditionExpression),
    conditionExpression(conditionExpression),
    trueBlock(trueBlock),
    falseBlock(0),
    elsePart(0)
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

Parameter::Parameter
    (const Token &nameToken, Expression *defaultExpression, bool isGroup) :
    name(nameToken.s),
    defaultExpression(defaultExpression),
    isGroup(isGroup)
{
    if (defaultExpression != 0 && isGroup)
        throw string("Group parameter cannot have a default");
}

Parameter::~Parameter()
{
    delete defaultExpression;
}

void Parameter::Parent(const Statement *statement)
{
    if (defaultExpression)
        defaultExpression->Parent(statement);
}

ParameterList::ParameterList()
{
}

void ParameterList::Add(Parameter *parameter)
{
    if (parameters.empty())
        (SourceElement &)*this = *parameter;
    parameters.push_back(parameter);
}

ParameterList::~ParameterList()
{
    for (auto iter = parameters.begin(); iter != parameters.end(); iter++)
        delete *iter;
}

void ParameterList::Parent(const Statement *statement)
{
    for (auto iter = parameters.begin(); iter != parameters.end(); iter++)
    {
        auto parameter = *iter;
        parameter->Parent(statement);
    }
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
