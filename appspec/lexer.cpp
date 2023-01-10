//
// Asp application specification lexical analyzer implementation.
//

#include "lexer.h"
#include "token-types.h"
#include <cctype>

using namespace std;

bool Lexer::keywordsInitialized = false;

Lexer::Lexer(istream &is) :
    is(is)
{
    // Add keywords specific to the application specification language.
    if (!keywordsInitialized)
    {
        keywordsInitialized = true;
        keywords.emplace("include", TOKEN_INCLUDE);
    }
}

Token *Lexer::Next()
{
    while (true)
    {
        sourceLocation = caret;

        // Check indent if applicable. Note that this may generate pending
        // tokens to indicate block-end or indent errors.
        int c = Peek();

        if (c == EOF)
            break;
        else if (c == '\n')
            return ProcessStatementEnd();
        else if (isdigit(c))
            return ProcessNumber();
        else if (isalpha(c) || c == '_')
            return ProcessName();
        else if (c == '\'' || c == '"')
            return ProcessString();
        else if (c == '.')
        {
            if (isdigit(Peek(1)))
                return ProcessNumber();
            else if (Peek(1) == '.' && Peek(2) == '.')
            {
                Get(); Get(); Get();
                return new Token(sourceLocation, TOKEN_ELLIPSIS);
            }
        }
        else if (c == '-' &&
                 (isdigit(Peek(1)) || Peek(1) == '.' && isdigit(Peek(2))))
        {
            return ProcessSignedNumber();
        }
        else if (c == '=')
        {
            Get();
            return new Token(sourceLocation, TOKEN_ASSIGN);
        }
        else if (c == ',')
        {
            Get();
            return new Token(sourceLocation, TOKEN_COMMA);
        }
        else if (c == '(')
        {
            Get();
            return new Token(sourceLocation, TOKEN_LEFT_PAREN);
        }
        else if (c == ')')
        {
            Get();
            return new Token(sourceLocation, TOKEN_RIGHT_PAREN);
        }
        else if (c == '*')
        {
            Get();
            return new Token(sourceLocation, TOKEN_ASTERISK);
        }
        else
        {
            Get();
            if (!isspace(c) && c != CHAR_LINE_CONTINUATION)
                return new Token(sourceLocation, -1, string(1, c));
        }
    }

    return new Token(sourceLocation);
}

Token *Lexer::ProcessSignedNumber()
{
    Get(); // -
    Token *result = ProcessNumber();
    switch (result->type)
    {
        case TOKEN_INTEGER:
            result->i = -result->i;
            break;

        case TOKEN_FLOAT:
            result->f = -result->f;
            break;
    }
    return result;
}

int Lexer::Get()
{
    // Get next character from peek prefetch or file as appropriate.
    int c;
    if (!prefetch.empty())
    {
        c = prefetch.front();
        prefetch.pop_front();
    }
    else
        c = Read();

    // Maintain line/column.
    if (c == '\n' || c == CHAR_LINE_CONTINUATION)
    {
        caret.column = 0;
        caret.line++;
    }
    caret.column++;

    return c == CHAR_LINE_CONTINUATION ? ' ' : c;
}
