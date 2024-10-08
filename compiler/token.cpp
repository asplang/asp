//
// Asp token implementation.
//

#include "token.h"
#include <token-types.h>

using namespace std;

Token::Token
    (const SourceLocation &sourceLocation, int type, const string &s,
     const string &error) :
    SourceElement(sourceLocation),
    type(type),
    s(s),
    error(error)
{
}

Token::Token
    (const SourceLocation &sourceLocation, int32_t value, bool negatedMin,
     const string &s) :
    SourceElement(sourceLocation),
    type(TOKEN_INTEGER),
    negatedMinInteger(negatedMin),
    i(value),
    s(s)
{
}

Token::Token
    (const SourceLocation &sourceLocation, double value,
     const string &s) :
    SourceElement(sourceLocation),
    type(TOKEN_FLOAT),
    f(value),
    s(s)
{
}

Token::Token(const SourceLocation &sourceLocation, const string &value) :
    SourceElement(sourceLocation),
    type(TOKEN_STRING),
    s(value)
{
}
