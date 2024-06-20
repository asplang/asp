//
// Asp grammar.
//

%start_symbol module

%token_prefix TOKEN_
%extra_context {Compiler *compiler}

%token_type {Token *}
%default_type {NonTerminal *}

%token_destructor {FREE_TOKEN($$);}
%default_destructor {FREE_NT((NonTerminal *)$$);}

// Token precedences from low to high.
%right ASSIGN.
%left INSERT.
%left COMMA.
%left COLON.
%left IF. // Conditional expression.
%nonassoc RANGE.
%right STEP.
%left OR.
%left AND.
%right NOT.
%left EQ NE LT LE GT GE IN NOT_IN IS IS_NOT.
%left ORDER.
%left BAR. // Bitwise OR.
%left CARET. // Bitwise XOR.
%left AMPERSAND. // Bitwise AND.
%left LEFT_SHIFT RIGHT_SHIFT.
%left PLUS MINUS.
%left ASTERISK SLASH FLOOR_DIVIDE PERCENT. // Multiply, divide, etc.
%right UNARY TILDE. // Unary PLUS & MINUS, bitwise NOT.
%right DOUBLE_ASTERISK. // Power.
%left PERIOD. // Member access.
%nonassoc GRAVE. // Unary symbol operator.
%left LEFT_PAREN LEFT_BRACKET LEFT_BRACE.
%nonassoc NAME.

// Reserved tokens.
%token ASSERT CLASS EXCEPT EXEC FINALLY LAMBDA NONLOCAL RAISE TRY WITH YIELD.
%token UNEXPECTED_INDENT MISSING_INDENT MISMATCHED_UNINDENT INCONSISTENT_WS.

// Tokens used in different contexts.
%fallback FOR_IN IN.
%fallback PARAM_DEFAULT ASSIGN.
%fallback STEP COLON.

%include
{
#include "compiler.h"
#include <stddef.h>
}

%syntax_error
{
    ACTION(ReportError, "Syntax error");
}

%parse_failure
{
    ACTION(ReportError, "Parse error");
}

%stack_overflow
{
    ACTION(ReportError, "Parser stack overflow");
}

module(result) ::= statements(statements).
{
    ACTION(MakeModule, statements);
    result = 0;
}

%type statements {Block *}

statements(result) ::= statements(block) statement(statement).
{
    result = ACTION(AddStatementToBlock, block, statement);
}

statements(result) ::= .
{
    result = ACTION(MakeEmptyBlock, 0);
}

%type statement {Statement *}

statement(result) ::= STATEMENT_END.
{
    result = 0;
}

statement(result) ::= compound_statement(statement).
{
    result = ACTION(AssignStatement, statement);
}

statement(result) ::= simple_statement(statement).
{
    result = ACTION(AssignStatement, statement);
}

%type compound_statement {Statement *}

compound_statement(result) ::= if(statement).
{
    result = ACTION(AssignStatement, (Statement *)statement);
}

compound_statement(result) ::= while(statement).
{
    result = ACTION(AssignStatement, (Statement *)statement);
}

compound_statement(result) ::= for(statement).
{
    result = ACTION(AssignStatement, (Statement *)statement);
}

compound_statement(result) ::= def(statement).
{
    result = ACTION(AssignStatement, (Statement *)statement);
}

%type if {IfStatement *}

if(result) ::=
    IF expression(expression) block(trueBlock) elif(ifStatement).
{
    result = ACTION
        (MakeIfElseIfStatement, expression, trueBlock, ifStatement);
}

if(result) ::=
    IF expression(expression) block(trueBlock) else(falseBlock).
{
    result = ACTION
        (MakeIfElseStatement, expression, trueBlock, falseBlock);
}

if(result) ::=
    IF expression(expression) block(trueBlock).
{
    result = ACTION(MakeIfStatement, expression, trueBlock);
}

%type elif {IfStatement *}

elif(result) ::=
    ELIF expression(expression) block(trueBlock) elif(ifStatement).
{
    result = ACTION
        (MakeIfElseIfStatement, expression, trueBlock, ifStatement);
}

elif(result) ::=
    ELIF expression(expression) block(trueBlock) else(falseBlock).
{
    result = ACTION
        (MakeIfElseStatement, expression, trueBlock, falseBlock);
}

elif(result) ::=
    ELIF expression(expression) block(trueBlock).
{
    result = ACTION(MakeIfStatement, expression, trueBlock);
}

%type else {Block *}

else(result) ::= ELSE block(block).
{
    result = ACTION(AssignBlock, block);
}

%type while {Statement *}

while(result) ::=
    WHILE expression(expression) block(trueBlock) else(falseBlock).
{
    result = ACTION(MakeWhileStatement, expression, trueBlock, falseBlock);
}

while(result) ::=
    WHILE expression(expression) block(trueBlock).
{
    result = ACTION(MakeWhileStatement, expression, trueBlock, 0);
}

%type for {Statement *}

for(result) ::=
    FOR target(targetExpression) FOR_IN expression(iterableExpression)
    block(trueBlock) else(falseBlock).
{
    result = ACTION
        (MakeForStatement,
         targetExpression, iterableExpression, trueBlock, falseBlock);
}

for(result) ::=
    FOR target(targetExpression) FOR_IN expression(iterableExpression)
    block(trueBlock).
{
    result = ACTION
        (MakeForStatement,
         targetExpression, iterableExpression, trueBlock, 0);
}

%type target {TargetExpression *}

target(result) ::= target1(targetExpression).
{
    result = ACTION(AssignTargetExpression, targetExpression);
}

target(result) ::= target2(targetExpression).
{
    result = ACTION(AssignTargetExpression, targetExpression);
}

%type target1 {TargetExpression *}

target1(result) ::=
    target1(leftTargetExpression) COMMA(operatorToken)
    target1(rightTargetExpression).
{
    result = ACTION
        (MakeTargetExpression, operatorToken,
         leftTargetExpression, rightTargetExpression);
}

target1(result) ::=
    LEFT_PAREN target1(targetExpression) RIGHT_PAREN.
{
    result = ACTION(MakeEnclosedTargetExpression, targetExpression);
}

target1(result) ::=
    LEFT_PAREN target2(targetExpression) RIGHT_PAREN.
{
    result = ACTION(MakeEnclosedTargetExpression, targetExpression);
}

target1(result) ::= LEFT_PAREN(token) RIGHT_PAREN.
{
    result = ACTION(MakeTargetExpression, token, 0, 0);
}

target1(result) ::= NAME(nameToken).
{
    result = ACTION(MakeTargetExpression, nameToken, 0, 0);
}

%type target2 {TargetExpression *}

target2(result) ::=
    target1(leftTargetExpression) COMMA(operatorToken).
{
    result = ACTION
        (MakeTargetExpression, operatorToken, leftTargetExpression, 0);
}

%type def {Statement *}

def(result) ::=
    DEF NAME(nameToken) LEFT_PAREN parameters(parameterList) RIGHT_PAREN
    block(block).
{
    result = ACTION(MakeDefStatement, nameToken, parameterList, block);
}

%type parameters {ParameterList *}

parameters(result) ::=
    parameters(parameterList) COMMA parameter(parameter).
{
    result = ACTION(AddParameterToList, parameterList, parameter);
}

parameters(result) ::= parameter(parameter).
{
    result = ACTION
        (AddParameterToList,
         ACTION(MakeEmptyParameterList, 0),
         parameter);
}

parameters(result) ::= .
{
    result = ACTION(MakeEmptyParameterList, 0);
}

%type parameter {Parameter *}

parameter(result) ::=
    NAME(nameToken) PARAM_DEFAULT expression1(defaultExpression).
{
    result = ACTION(MakeParameter, nameToken, defaultExpression);
}

parameter(result) ::= NAME(nameToken).
{
    result = ACTION(MakeParameter, nameToken, 0);
}

parameter(result) ::=
    ASTERISK NAME(nameToken).
{
    result = ACTION(MakeTupleGroupParameter, nameToken);
}

parameter(result) ::=
    DOUBLE_ASTERISK NAME(nameToken).
{
    result = ACTION(MakeDictionaryGroupParameter, nameToken);
}

%type block {Block *}

block(result) ::= BLOCK_START statements(block) BLOCK_END.
{
    result = ACTION(AssignBlock, block);
}

block(result) ::= COLON simple_statement(statement). [BLOCK_START]
{
    result = ACTION
        (AddStatementToBlock,
         ACTION(MakeEmptyBlock, 0),
         statement);
}

%type simple_statement {Statement *}

simple_statement(result) ::= simple_statement_stem(stem) STATEMENT_END.
{
    result = ACTION(AssignStatement, stem);
}

%type simple_statement_stem {Statement *}

simple_statement_stem(result) ::= assignment(assignmentStatement).
{
    result = ACTION(MakeAssignmentStatement, assignmentStatement);
}

simple_statement_stem(result) ::= insertion(insertionStatement).
{
    result = ACTION(MakeInsertionStatement, insertionStatement);
}

simple_statement_stem(result) ::= expression(expression).
{
    result = ACTION(MakeExpressionStatement, expression);
}

simple_statement_stem(result) ::= import(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= global(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= local(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= del(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= return(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= assert(statement).
{
    result = ACTION(AssignStatement, statement);
}

simple_statement_stem(result) ::= BREAK(token).
{
    result = ACTION(MakeBreakStatement, token);
}

simple_statement_stem(result) ::= CONTINUE(token).
{
    result = ACTION(MakeContinueStatement, token);
}

simple_statement_stem(result) ::= PASS(token).
{
    result = ACTION(MakePassStatement, token);
}

%type assignment {AssignmentStatement *}

assignment(result) ::= simple_assignment(assignmentStatement).
{
    result = ACTION
        (AssignAssignmentStatement, assignmentStatement);
}

assignment(result) ::= augmented_assignment(assignmentStatement).
{
    result = ACTION
        (AssignAssignmentStatement, assignmentStatement);
}

%type simple_assignment {AssignmentStatement *}

simple_assignment(result) ::=
    expression(targetExpression) ASSIGN(assignmentToken)
    simple_assignment(valueAssignment).
{
    result = ACTION
        (MakeMultipleAssignmentStatement, assignmentToken,
         targetExpression, valueAssignment);
}

simple_assignment(result) ::=
    expression(targetExpression) ASSIGN(assignmentToken)
    expression(valueExpression).
{
    result = ACTION
        (MakeSingleAssignmentStatement, assignmentToken,
         targetExpression, valueExpression);
}

%type augmented_assignment {AssignmentStatement *}

augmented_assignment(result) ::=
    expression(targetExpression) augmented_assign(assignmentToken)
    expression(valueExpression). [ASSIGN]
{
    result = ACTION
        (MakeSingleAssignmentStatement, assignmentToken,
         targetExpression, valueExpression);
}

%type augmented_assign {Token *}

augmented_assign(result) ::= BIT_OR_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= BIT_XOR_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= BIT_AND_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= LEFT_SHIFT_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= RIGHT_SHIFT_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= PLUS_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= MINUS_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= TIMES_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= DIVIDE_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= FLOOR_DIVIDE_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= MODULO_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

augmented_assign(result) ::= POWER_ASSIGN(token).
{
    result = ACTION(AssignToken, token);
}

%type insertion {InsertionStatement *}

insertion(result) ::=
    insertion(insertionStatement) INSERT(insertionToken)
    expression(itemExpression).
{
    result = ACTION
        (MakeMultipleValueInsertionStatement, insertionToken,
         insertionStatement, itemExpression);
}

insertion(result) ::=
    insertion(insertionStatement) INSERT(insertionToken)
    key_value_pair(keyValuePair).
{
    result = ACTION
        (MakeMultiplePairInsertionStatement, insertionToken,
         insertionStatement, keyValuePair);
}

insertion(result) ::=
    expression(containerExpression) INSERT(insertionToken)
    expression(itemExpression).
{
    result = ACTION
        (MakeSingleValueInsertionStatement, insertionToken,
         containerExpression, itemExpression);
}

insertion(result) ::=
    expression(containerExpression) INSERT(insertionToken)
    key_value_pair(keyValuePair).
{
    result = ACTION
        (MakeSinglePairInsertionStatement, insertionToken,
         containerExpression, keyValuePair);
}

%type expression {Expression *}

expression(result) ::= expression1(expression).
{
    result = ACTION(AssignExpression, expression);
}

expression(result) ::= expression2(expression).
{
    result = ACTION(AssignExpression, expression);
}

%type expression1 {Expression *}

expression1(result) ::=
    expression1(leftExpression) COMMA(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeTupleExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(trueExpression) IF(operatorToken)
    expression1(conditionExpression) ELSE expression1(falseExpression).
{
    result = ACTION
        (MakeConditionalExpression, operatorToken,
         conditionExpression, trueExpression, falseExpression);
}

expression1(result) ::= range(rangeExpression). [RANGE]
{
    result = ACTION(MakeRangeExpression, rangeExpression);
}

expression1(result) ::=
    expression1(leftExpression) OR(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeShortCircuitLogicalExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) AND(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeShortCircuitLogicalExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    NOT(operatorToken) expression1(expression).
{
    result = ACTION
        (MakeUnaryExpression, operatorToken, expression);
}

expression1(result) ::=
    expression1(leftExpression) EQ(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) NE(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) LT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) LE(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) GT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) GE(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) IN(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) NOT_IN(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) ORDER(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) IS(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) IS_NOT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) BAR(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) CARET(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) AMPERSAND(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) LEFT_SHIFT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) RIGHT_SHIFT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) PLUS(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) MINUS(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) ASTERISK(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) SLASH(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) FLOOR_DIVIDE(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    expression1(leftExpression) PERCENT(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    PLUS(operatorToken) expression1(expression). [UNARY]
{
    result = ACTION
        (MakeUnaryExpression, operatorToken, expression);
}

expression1(result) ::=
    MINUS(operatorToken) expression1(expression). [UNARY]
{
    result = ACTION
        (MakeUnaryExpression, operatorToken, expression);
}

expression1(result) ::=
    TILDE(operatorToken) expression1(expression).
{
    result = ACTION
        (MakeUnaryExpression, operatorToken, expression);
}

expression1(result) ::=
    expression1(leftExpression) DOUBLE_ASTERISK(operatorToken)
    expression1(rightExpression).
{
    result = ACTION
        (MakeBinaryExpression, operatorToken,
         leftExpression, rightExpression);
}

expression1(result) ::=
    LEFT_PAREN expression1(expression) RIGHT_PAREN.
{
    result = ACTION(MakeEnclosedExpression, expression);
}

expression1(result) ::=
    LEFT_PAREN expression2(expression) RIGHT_PAREN.
{
    result = ACTION(MakeEnclosedExpression, expression);
}

expression1(result) ::= LEFT_PAREN(token) RIGHT_PAREN.
{
    result = ACTION(MakeTupleExpression, token, 0, 0);
}

expression1(result) ::=
    expression1(functionExpression)
    LEFT_PAREN arguments(argumentList) RIGHT_PAREN.
{
    result = ACTION
        (MakeCallExpression, functionExpression, argumentList);
}

expression1(result) ::=
    expression1(sequenceExpression)
    LEFT_BRACKET expression1(indexExpression) RIGHT_BRACKET.
{
    result = ACTION
        (MakeElementExpression, sequenceExpression, indexExpression);
}

expression1(result) ::= expression1(expression) PERIOD NAME(nameToken).
{
    result = ACTION
        (MakeMemberExpression, expression, nameToken);
}

expression1(result) ::= dictionary(dictionaryExpression).
{
    result = ACTION(MakeDictionaryExpression, dictionaryExpression);
}

expression1(result) ::= set(setExpression).
{
    result = ACTION(MakeSetExpression, setExpression);
}

expression1(result) ::= list(listExpression).
{
    result = ACTION(MakeListExpression, listExpression);
}

expression1(result) ::= GRAVE(operatorToken) NAME(nameToken).
{
    result = ACTION
        (MakeSymbolExpression, operatorToken, nameToken);
}

expression1(result) ::= literal(literal).
{
    result = ACTION(MakeLiteralExpression, literal);
}

expression1(result) ::= NAME(nameToken).
{
    result = ACTION(MakeVariableExpression, nameToken);
}

%type expression2 {Expression *}

expression2(result) ::= expression1(leftExpression) COMMA(operatorToken).
{
    result = ACTION
        (MakeTupleExpression, operatorToken, leftExpression, 0);
}

%type arguments {ArgumentList *}

arguments(result) ::=
    arguments(argumentList) COMMA argument(argument).
{
    result = ACTION(AddArgumentToList, argumentList, argument);
}

arguments(result) ::= argument(argument).
{
    result = ACTION
        (AddArgumentToList,
         ACTION(MakeEmptyArgumentList, 0),
         argument);
}

arguments(result) ::= .
{
    result = ACTION(MakeEmptyArgumentList, 0);
}

%type argument {Argument *}

argument(result) ::=
    NAME(nameToken) ASSIGN expression1(valueExpression).
{
    result = ACTION(MakeArgument, nameToken, valueExpression);
}

argument(result) ::= expression1(valueExpression). [NAME]
{
    result = ACTION(MakeArgument, 0, valueExpression);
}

argument(result) ::= ASTERISK expression1(valueExpression). [NAME]
{
    result = ACTION(MakeIterableGroupArgument, valueExpression);
}

argument(result) ::= DOUBLE_ASTERISK expression1(valueExpression). [NAME]
{
    result = ACTION(MakeDictionaryGroupArgument, valueExpression);
}

%type range {RangeExpression *}

range(result) ::=
    range_part(startExpression)
    RANGE(token) range_part(endExpression)
    STEP expression1(stepExpression). [STEP]
{
    result = ACTION
        (MakeRange, token, startExpression, endExpression, stepExpression);
}

range(result) ::=
    range_part(startExpression)
    RANGE(token) range_part(endExpression). [STEP]
{
    result = ACTION
        (MakeRange, token, startExpression, endExpression, 0);
}

%type range_part {Expression *}

range_part(result) ::= expression1(expression). [RANGE]
{
    result = ACTION(AssignExpression, expression);
}

range_part(result) ::= . [RANGE]
{
    result = 0;
}

%type import {Statement *}

import(result) ::= IMPORT import_names(importNameList).
{
    result = ACTION(MakeImportStatement, importNameList);
}

import(result) ::=
    FROM NAME(moduleNameToken) IMPORT import_names(importNameList).
{
    result = ACTION
        (MakeFromImportStatement,
         ACTION(MakeImportName, moduleNameToken, 0),
         importNameList);
}

import(result) ::=
    FROM NAME(moduleNameToken) IMPORT ASTERISK(wildcardToken).
{
    result = ACTION
        (MakeFromImportStatement,
         ACTION(MakeImportName, moduleNameToken, 0),
         ACTION
            (AddImportNameToList,
             ACTION(MakeEmptyImportNameList, 0),
             ACTION(MakeImportName, wildcardToken, 0)));
}

%type import_names {ImportNameList *}

import_names(result) ::=
    import_names(importNameList) COMMA import_name(importName).
{
    result = ACTION
        (AddImportNameToList, importNameList, importName);
}

import_names(result) ::= import_name(importName).
{
    result = ACTION
        (AddImportNameToList,
         ACTION(MakeEmptyImportNameList, 0),
         importName);
}

%type import_name {ImportName *}

import_name(result) ::= NAME(nameToken) AS NAME(asNameToken).
{
    result = ACTION(MakeImportName, nameToken, asNameToken);
}

import_name(result) ::= NAME(nameToken).
{
    result = ACTION(MakeImportName, nameToken, 0);
}

%type global {Statement *}

global(result) ::= GLOBAL variables(variableList).
{
    result = ACTION(MakeGlobalStatement, variableList);
}

%type local {Statement *}

local(result) ::= LOCAL variables(variableList).
{
    result = ACTION(MakeLocalStatement, variableList);
}

%type variables {VariableList *}

variables(result) ::=
    variables(variableList) COMMA NAME(nameToken).
{
    result = ACTION
        (AddVariableToList, variableList, nameToken);
}

variables(result) ::= NAME(nameToken).
{
    result = ACTION
        (AddVariableToList,
         ACTION(MakeEmptyVariableList, 0),
         nameToken);
}

%type del {Statement *}

del(result) ::= DEL expression(expression).
{
    result = ACTION(MakeDelStatement, expression);
}

%type return {Statement *}

return(result) ::= RETURN(keywordToken) expression(expression).
{
    result = ACTION(MakeReturnStatement, keywordToken, expression);
}

return(result) ::= RETURN(keywordToken).
{
    result = ACTION(MakeReturnStatement, keywordToken, 0);
}

%type assert {Statement *}

assert(result) ::= ASSERT expression(expression).
{
    result = ACTION(MakeAssertStatement, expression);
}

%type set {SetExpression *}

set(result) ::= LEFT_BRACE(token) RIGHT_BRACE.
{
    result = ACTION(MakeEmptySet, token);
}

set(result) ::=
    LEFT_BRACE(token) elements(keyList) RIGHT_BRACE.
{
    result = ACTION(AssignSet, token, keyList);
}

%type dictionary {DictionaryExpression *}

dictionary(result) ::= LEFT_BRACE(token) COLON RIGHT_BRACE.
{
    result = ACTION(MakeEmptyDictionary, token);
}

dictionary(result) ::=
    LEFT_BRACE dictionary_entries(dictionaryEntriesExpression) RIGHT_BRACE.
{
    result = ACTION(AssignDictionary, dictionaryEntriesExpression);
}

%type dictionary_entries {DictionaryExpression *}

dictionary_entries(result) ::= dictionary_entries(dictionaryExpression) COMMA.
{
    result = ACTION(AssignDictionary, dictionaryExpression);
}

dictionary_entries(result) ::=
    dictionary_entries(dictionaryExpression) COMMA
    key_value_pair(keyValuePair).
{
    result = ACTION
        (AddEntryToDictionary, dictionaryExpression, keyValuePair);
}

dictionary_entries(result) ::= key_value_pair(keyValuePair).
{
    result = ACTION
        (AddEntryToDictionary,
         ACTION(MakeEmptyDictionary, 0),
         keyValuePair);
}

%type key_value_pair {KeyValuePair *}

key_value_pair(result) ::=
    expression1(keyExpression) COLON expression1(valueExpression).
{
    result = ACTION
        (MakeKeyValuePair, keyExpression, valueExpression);
}

%type list {ListExpression *}

list(result) ::= LEFT_BRACKET(token) RIGHT_BRACKET.
{
    result = ACTION(MakeEmptyList, token);
}

list(result) ::=
    LEFT_BRACKET(token) elements(elementList) RIGHT_BRACKET.
{
    result = ACTION(AssignList, token, elementList);
}

%type elements {ListExpression *}

elements(result) ::= elements(elementList) COMMA.
{
    result = ACTION(AssignList, 0, elementList);
}

elements(result) ::=
    elements(listExpression) COMMA expression1(elementExpression).
{
    result = ACTION
        (AddExpressionToList, listExpression, elementExpression);
}

elements(result) ::= expression1(elementExpression). [COMMA]
{
    result = ACTION
        (AddExpressionToList,
         ACTION(MakeEmptyList, 0),
         elementExpression);
}

%type literal {ConstantExpression *}

literal(result) ::= NONE(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= FALSE(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= TRUE(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= ELLIPSIS(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= INTEGER(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= FLOAT(token).
{
    result = ACTION(MakeConstantExpression, token);
}

literal(result) ::= string_literal(literal).
{
    result = ACTION(AssignConstantExpression, literal);
}

%type string_literal {ConstantExpression *}

string_literal(result) ::= string_literal(literal) STRING(token).
{
    result = ACTION
        (MakeJuxtaposeConstantExpression,
         literal,
         ACTION(MakeConstantExpression, token));
}

string_literal(result) ::= STRING(token).
{
    result = ACTION(MakeConstantExpression, token);
}
