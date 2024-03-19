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
            type = AppSpecValueType_None;
            break;

        case TOKEN_ELLIPSIS:
            type = AppSpecValueType_Ellipsis;
            break;

        case TOKEN_FALSE:
            type = AppSpecValueType_Boolean;
            b = false;
            break;

        case TOKEN_TRUE:
            type = AppSpecValueType_Boolean;
            b = true;
            break;

        case TOKEN_INTEGER:
            if (token.negatedMinInteger)
                throw string("Integer constant out of range");
            type = AppSpecValueType_Integer;
            i = token.i;
            break;

        case TOKEN_FLOAT:
            type = AppSpecValueType_Float;
            f = token.f;
            break;

        case TOKEN_STRING:
            type = AppSpecValueType_String;
            s = token.s;
            break;
    }
}
