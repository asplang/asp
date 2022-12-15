//
// Asp lexical analyzer implementation - common.
//

#include <lexer.h>
#include <token-types.h>
#include <cctype>
#include <map>
#include <limits>
#include <cstdlib>

using namespace std;

Lexer::Lexer(istream &is) :
    is(is)
{
}

Token *Lexer::ProcessStatementEnd()
{
    Get(); // ; or newline
    return new Token(sourceLocation, TOKEN_STATEMENT_END);
}

Token *Lexer::ProcessNumber()
{
    enum class State
    {
        Null,
        Zero,
        Decimal,
        IncompleteHexadecimal,
        Hexadecimal,
        IncompleteSimpleFloat,
        SimpleFloat,
        IncompleteExponential,
        IncompleteSignedExponential,
        Exponential,
        Invalid,
    } state = State::Null;
    string lex;
    while (!is.eof())
    {
        auto c = Peek();

        bool end = false;
        switch (state)
        {
            case State::Null:
                if (c == '0')
                    state = State::Zero;
                else if (c == '.')
                    state = State::IncompleteSimpleFloat;
                else if (isdigit(c))
                    state = State::Decimal;
                else
                    state = State::Invalid;
                break;

            case State::Zero:
                if (tolower(c) == 'x')
                    state = State::IncompleteHexadecimal;
                else if (c == '.')
                {
                    if (Peek(1) == '.')
                        end = true;
                    else
                        state = State::SimpleFloat;
                }
                else if (tolower(c) == 'e')
                    state = State::IncompleteExponential;
                else if (isdigit(c))
                    state = State::Decimal;
                else if (isalpha(c))
                    state = State::Invalid;
                else
                    end = true;
                break;

            case State::Decimal:
                if (c == '.')
                {
                    if (Peek(1) == '.')
                        end = true;
                    else
                        state = State::SimpleFloat;
                }
                else if (tolower(c) == 'e')
                    state = State::IncompleteExponential;
                else if (isalpha(c))
                    state = State::Invalid;
                else if (!isdigit(c))
                    end = true;
                break;

            case State::IncompleteHexadecimal:
                if (isxdigit(c))
                    state = State::Hexadecimal;
                else
                    state = State::Invalid;
                break;

            case State::Hexadecimal:
                if (!isxdigit(c))
                {
                    if (isalpha(c))
                        state = State::Invalid;
                    if (c == '.')
                    {
                        if (Peek(1) == '.')
                            end = true;
                        else
                            state = State::Invalid;
                    }
                    else
                        end = true;
                }
                break;

            case State::IncompleteSimpleFloat:
                if (isdigit(c))
                    state = State::SimpleFloat;
                else
                    state = State::Invalid;
                break;

            case State::SimpleFloat:
                if (tolower(c) == 'e')
                    state = State::IncompleteExponential;
                else if (isalpha(c))
                    state = State::Invalid;
                else if (c == '.')
                {
                    if (Peek(1) == '.')
                        end = true;
                    else
                        state = State::Invalid;
                }
                else if (!isdigit(c))
                    end = true;
                break;

            case State::IncompleteExponential:
                if (c == '+' || c == '-')
                    state = State::IncompleteSignedExponential;
                else if (isdigit(c))
                    state = State::Exponential;
                else
                    state = State::Invalid;
                break;

            case State::IncompleteSignedExponential:
                if (isdigit(c))
                    state = State::Exponential;
                else
                    state = State::Invalid;
                break;

            case State::Exponential:
                if (isalpha(c))
                    state = State::Invalid;
                if (c == '.')
                {
                    if (Peek(1) == '.')
                        end = true;
                    else
                        state = State::Invalid;
                }
                else if (!isdigit(c))
                    end = true;
                break;
        }

        if (end || state == State::Invalid)
            break;

        lex += static_cast<char>(c);
        Get();
    }

    auto s = lex.c_str();
    switch (state)
    {
        case State::Zero:
        case State::Decimal:
        {
            auto value = static_cast<unsigned>(strtoul(s, 0, 10));
            return new Token
                (sourceLocation, *reinterpret_cast<int *>(&value), 0, lex);
        }

        case State::Hexadecimal:
        {
            auto value = static_cast<unsigned>(strtoul(s, 0, 0x10));
            return new Token
                (sourceLocation, *reinterpret_cast<int *>(&value), 0, lex);
        }

        case State::SimpleFloat:
        case State::Exponential:
            return new Token(sourceLocation, strtod(s, 0), lex);
    }

    return new Token(sourceLocation, -1, lex);
}

Token *Lexer::ProcessString()
{
    string lex, escapeLex;

    char quote = static_cast<char>(Get());

    enum class State
    {
        End,
        Normal,
        CharacterEscape,
        DecimalEscape,
        HexadecimalEscape,
        HexadecimalEscapeDigits,
    } state = State::Normal;

    bool badString = false;
    while (!badString && state != State::End)
    {
        int ci = Get();
        if (ci == EOF)
            break;
        char c = static_cast<char>(ci);

        switch (state)
        {
            default: // No escape sequence
                if (c == '\n')
                    badString = true;
                else if (c == '\\')
                {
                    // Look ahead to determine next state.
                    int c2 = Peek();
                    if (c2 == EOF)
                        badString = true;
                    else if (isdigit(c2))
                        state = State::DecimalEscape;
                    else if (c2 == 'x')
                        state = State::HexadecimalEscape;
                    else
                        state = State::CharacterEscape;
                }
                else if (c == quote)
                    state = State::End;
                else
                    lex += c;
                break;

            case State::CharacterEscape:
                switch (c)
                {
                    case 'a':
                        c = '\a';
                        break;
                    case 'b':
                        c = '\b';
                        break;
                    case 'f':
                        c = '\f';
                        break;
                    case 'n':
                    case '\n':
                        c = '\n';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case 'v':
                        c = '\v';
                        break;
                    case '\\':
                        c = '\\';
                        break;
                    case '"':
                        c = '"';
                        break;
                    case '\'':
                        c = '\'';
                        break;
                    default:
                        badString = true;
                        break;
                }
                lex += c;
                state = State::Normal;
                break;

            case State::DecimalEscape:
            {
                escapeLex += c;
                char c2 = static_cast<char>(Peek());
                if (escapeLex.size() == 3 || !isdigit(c2))
                {
                    int value = atoi(escapeLex.c_str());
                    if (value > 0xFF)
                        badString = true;
                    else
                    {
                        lex += static_cast<char>(value);
                        escapeLex.clear();
                        state = State::Normal;
                    }
                }
                break;
            }

            case State::HexadecimalEscape:
                state = State::HexadecimalEscapeDigits;
                break;

            case State::HexadecimalEscapeDigits:
                escapeLex += c;
                if (!isxdigit(c))
                    badString = true;
                else if (escapeLex.size() == 2)
                {
                    lex += static_cast<char>(strtol(escapeLex.c_str(), 0, 16));
                    escapeLex.clear();
                    state = State::Normal;
                }
                break;
        }
    }

    if (state != State::End)
        return new Token
            (sourceLocation, -1, !escapeLex.empty() ? escapeLex : lex);

    return new Token(sourceLocation, lex);
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
    int type;
    auto iter = keywords.find(lex);
    return
        iter != keywords.end() ?
        new Token(sourceLocation, iter->second, lex) :
        new Token(sourceLocation, TOKEN_NAME, lex);
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

    // Ensure the last character ends a line.
    if (c == EOF)
    {
        readahead.push_back(c);
        c = '\n';
    }

    return c;
}
