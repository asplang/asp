//
// Asp literal implementation.
//

#include "literal.hpp"
#include "app.h"

using namespace std;

Literal::Literal(const Token &token)
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
