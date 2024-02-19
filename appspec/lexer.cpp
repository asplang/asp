//
// Asp application specification lexical analyzer implementation.
//

#include "lexer.h"
#include "token-types.h"
#include <cctype>

using namespace std;

bool Lexer::keywordsInitialized = false;

Lexer::Lexer(istream &is, const string &fileName) :
    is(is),
    caret(fileName, 1, 1)
{
    // Add keywords specific to the application specification language.
    if (!keywordsInitialized)
    {
        keywordsInitialized = true;
        keywords.emplace("include", TOKEN_INCLUDE);
        keywords.emplace("lib", TOKEN_LIB);
    }
}

Token *Lexer::Next()
{
    Token *token = nullptr;
    while (token == nullptr)
    {
        sourceLocation = caret;

        // Check indent if applicable. Note that this may generate pending
        // tokens to indicate block-end or indent errors.
        int c = Peek();

        if (c == EOF)
            token = new Token(sourceLocation);
        else if (c == '\\')
            token = ProcessLineContinuation();
        else if (c == '#')
            token = ProcessComment();
        else if (c == '\n')
            token = ProcessStatementEnd();
        else if (isdigit(c))
            token = ProcessNumber();
        else if (isalpha(c) || c == '_')
            token = ProcessName();
        else if (c == '\'' || c == '"')
            token = ProcessString();
        else if (c == '-' &&
                 (isdigit(Peek(1)) || Peek(1) == '.' && isdigit(Peek(2))))
            token = ProcessNumber();
        else if (c == '.')
        {
            if (Peek(1) == '.' && Peek(2) == '.')
            {
                Get(); Get(); Get();
                token = new Token(sourceLocation, TOKEN_ELLIPSIS);
            }
            else
                token = ProcessNumber();
        }
        else if (c == '=')
        {
            Get();
            token = new Token(sourceLocation, TOKEN_ASSIGN);
        }
        else if (c == ',')
        {
            Get();
            token = new Token(sourceLocation, TOKEN_COMMA);
        }
        else if (c == '(')
        {
            Get();
            token = new Token(sourceLocation, TOKEN_LEFT_PAREN);
        }
        else if (c == ')')
        {
            Get();
            token = new Token(sourceLocation, TOKEN_RIGHT_PAREN);
        }
        else if (c == '*')
        {
            if (Peek(1) == '*')
            {
                Get(); Get();
                token = new Token(sourceLocation, TOKEN_DOUBLE_ASTERISK);
            }
            else
            {
                Get();
                token = new Token(sourceLocation, TOKEN_ASTERISK);
            }
        }
        else
        {
            Get();
            if (!isspace(c))
                token = new Token(sourceLocation, -1, string(1, c));
        }
    }

    return token;
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
    if (c == '\n')
    {
        caret.column = 0;
        caret.line++;
    }
    caret.column++;

    return c;
}
