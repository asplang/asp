//
// Asp application specification grammar.
//

%start_symbol spec

%token_prefix TOKEN_
%extra_context {Generator *generator}

%token_type {Token *}
%default_type {NonTerminal *}

%token_destructor {FREE_TOKEN($$);}
%default_destructor {FREE_NT((NonTerminal *)$$);}

// Token precedences from low to high.
%right ASSIGN.
%left LEFT_PAREN.
%nonassoc NAME.

// Reserved keyword tokens.
%token AND ASSERT BREAK CLASS CONTINUE DEL ELIF ELSE EXCEPT EXEC.
%token FINALLY FOR FROM GLOBAL IF IN IS LAMBDA LOCAL NONLOCAL NOT OR.
%token PASS RAISE RETURN TRY WHILE WITH YIELD.
%token UNEXPECTED_INDENT MISSING_INDENT MISMATCHED_UNINDENT INCONSISTENT_WS.

// Optional tokens.
%token BLOCK_START.

%include
{
#include "generator.h"
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

spec(result) ::= statements.
{
    result = 0;
}

statements(result) ::= statements statement.
{
    result = 0;
}

statements(result) ::= .
{
    result = 0;
}

statement(result) ::= LINE_END.
{
    result = 0;
}

statement(result) ::= LIB(command) LINE_END.
{
    result = ACTION(DeclareAsLibrary, command);
}

statement(result) ::= INCLUDE NAME(includeName) LINE_END.
{
    result = ACTION(IncludeHeader, includeName);
}

statement(result) ::= INCLUDE STRING(includeString) LINE_END.
{
    result = ACTION(IncludeHeader, includeString);
}

statement(result) ::= IMPORT NAME(importName) LINE_END.
{
    result = ACTION(ImportModule, importName, importName);
}

statement(result) ::=
    IMPORT NAME(importName) AS NAME(asNameToken) LINE_END.
{
    result = ACTION(ImportModule, importName, asNameToken);
}

statement(result) ::=
    IMPORT STRING(importString) AS NAME (asNameToken) LINE_END.
{
    result = ACTION(ImportModule, importString, asNameToken);
}

statement(result) ::=
    PLUS NAME(featureToken) LINE_END.
{
    result = ACTION(UpdateFeatures, featureToken, 1);
}

statement(result) ::=
    MINUS NAME(featureToken) LINE_END.
{
    result = ACTION(UpdateFeatures, featureToken, 0);
}

%ifdef ASP_FEATURE_CLASS

statement(result) ::=
    PLUS CLASS(featureToken) LINE_END.
{
    result = ACTION(UpdateFeatures, featureToken, 1);
}

statement(result) ::=
    MINUS CLASS(featureToken) LINE_END.
{
    result = ACTION(UpdateFeatures, featureToken, 0);
}

%endif

statement(result) ::=
    NAME(nameToken) ASSIGN literal(value) LINE_END.
{
    result = ACTION(MakeAssignment, nameToken, value);
}

statement(result) ::=
    DEF NAME(nameToken) LEFT_PAREN parameters(parameterList) RIGHT_PAREN
    ASSIGN NAME(internalName) LINE_END.
{
    result = ACTION(MakeFunction, nameToken, parameterList, internalName);
}

%ifdef ASP_FEATURE_CLASS

statement(result) ::=
    CLASS NAME(nameToken) BLOCK_START.
{
    result = ACTION(StartClass, nameToken);
}

%endif

statement(result) ::= BLOCK_END.
{
    result = ACTION(EndBlock, 0);
}

statement(result) ::= NAME(nameToken) LINE_END.
{
    result = ACTION(MakeAssignment, nameToken, 0);
}

statement(result) ::= DEL names(nameList) LINE_END.
{
    result = ACTION(DeleteDefinition, nameList);
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
    NAME(nameToken) ASSIGN literal(defaultValue).
{
    result = ACTION(MakeDefaultedParameter, nameToken, defaultValue);
}

parameter(result) ::= NAME(nameToken).
{
    result = ACTION(MakeParameter, nameToken);
}

parameter(result) ::= ASTERISK NAME(nameToken).
{
    result = ACTION(MakeTupleGroupParameter, nameToken);
}

parameter(result) ::= DOUBLE_ASTERISK NAME(nameToken).
{
    result = ACTION(MakeDictionaryGroupParameter, nameToken);
}

%type names {NameList *}

names(result) ::= names(nameList) COMMA NAME(name).
{
    result = ACTION(AddNameToList, nameList, name);
}

names(result) ::= NAME(name).
{
    result = ACTION
        (AddNameToList,
         ACTION(MakeEmptyNameList, 0),
         name);
}

%type literal {Literal *}

literal(result) ::= NONE(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= FALSE(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= TRUE(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= ELLIPSIS(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= INTEGER(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= FLOAT(token).
{
    result = ACTION(MakeLiteral, token);
}

literal(result) ::= STRING(token).
{
    result = ACTION(MakeLiteral, token);
}
