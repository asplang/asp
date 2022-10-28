//
// Asp application specification lexical analyzer implementation.
//

#include "lexer.h"
#include "app.h"
#include <cctype>
#include <cstring>
#include <map>
#include <algorithm>
#include <limits>

using namespace std;

const int CHAR_LINE_CONTINUATION = 0x100;

Lexer::Lexer(istream &is) :
    is(is)
{
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
        else if (isalpha(c) || c == '_')
            return ProcessName();
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

Token *Lexer::ProcessStatementEnd()
{
    Get(); // newline
    return new Token(sourceLocation, TOKEN_STATEMENT_END);
}

Token *Lexer::ProcessName()
{
    string lex;
    while (!is.eof())
    {
        auto c = Peek();
        if (!isalpha(c) && !isdigit(c) && c != '_')
            break;
        lex += static_cast<char>(c);
        Get();
    }

    // Extract keywords.
    static const map<string, int> keywords =
    {
        {"def", TOKEN_DEF},
        {"include", TOKEN_INCLUDE},
    };
    int type;
    auto iter = keywords.find(lex);
    return
        iter != keywords.end() ?
        new Token(sourceLocation, iter->second, lex) :
        new Token(sourceLocation, TOKEN_NAME, lex);
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

int Lexer::Peek(unsigned n)
{
    if (n < prefetch.size())
        return prefetch[n];

    int c = EOF;
    for (n -= prefetch.size(), n++; !is.eof() && n > 0; n--)
    {
        c = Read();
        prefetch.push_back(c);
    }

    return c;
}

int Lexer::Read()
{
    int c;
    if (!readahead.empty())
    {
        c = readahead.front();
        readahead.pop_front();
        if (c == EOF)
            return c;
    }
    else
        c = is.get();

    // Join continued lines with an intervening space.
    if (c == '\\')
    {
        int c2 = is.get();
        if (c2 == '\n')
            c = CHAR_LINE_CONTINUATION;
        else
        {
            if (c2 == EOF)
                c = '\n';
            readahead.push_back(c2);
        }
    }

    // Discard comments.
    if (c == '#')
    {
        is.ignore(numeric_limits<streamsize>::max(), '\n');
        c = is.eof() ? EOF : '\n';
    }

    // Ensure last character ends a line.
    if (c == EOF)
    {
        readahead.push_back(c);
        c = '\n';
    }

    return c;
}

Token::Token
    (const SourceLocation &sourceLocation, int type, const string &s) :
    SourceElement(sourceLocation),
    type(type),
    s(s)
{
}
