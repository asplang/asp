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
%token AND AS ASSERT BREAK CLASS CONTINUE DEL ELIF ELSE EXCEPT EXEC.
%token FINALLY FOR FROM GLOBAL IF IMPORT IN IS LAMBDA LOCAL NONLOCAL NOT OR.
%token PASS RAISE RETURN TRY WHILE WITH YIELD.

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

statement(result) ::= LIB STATEMENT_END.
{
    result = ACTION(DeclareAsLibrary, 0);
}

statement(result) ::= INCLUDE NAME(includeName) STATEMENT_END.
{
    result = ACTION(IncludeHeader, includeName);
}

statement(result) ::=
    NAME(nameToken) ASSIGN literal(value).
{
    result = ACTION(MakeAssignment, nameToken, value);
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
