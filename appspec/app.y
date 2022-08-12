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

statement(result) ::= STATEMENT_END.
{
    result = 0;
}

statement(result) ::= INCLUDE NAME(includeName) STATEMENT_END.
{
    result = ACTION(IncludeHeader, includeName);
}

statement(result) ::=
    DEF NAME(nameToken) LEFT_PAREN parameters(parameterList) RIGHT_PAREN
    ASSIGN NAME(internalName).
{
    result = ACTION
        (MakeFunction, nameToken, parameterList,
         internalName);
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

parameter(result) ::= NAME(nameToken).
{
    result = ACTION(MakeParameter, nameToken);
}
