//
// Asp lexical analyzer implementation - common.
//

#include <lexer.h>
#include <token-types.h>
#include <cstdint>
#include <cctype>
#include <map>
#include <limits>
#include <cstdlib>
#include <cerrno>

using namespace std;

// Keywords, used and reserved.
map<string, int> Lexer::keywords =
{
    {"and", TOKEN_AND},
    {"as", TOKEN_AS},
    {"assert", TOKEN_ASSERT},
    {"break", TOKEN_BREAK},
    {"class", TOKEN_CLASS},
    {"continue", TOKEN_CONTINUE},
    {"def", TOKEN_DEF},
    {"del", TOKEN_DEL},
    {"elif", TOKEN_ELIF},
    {"else", TOKEN_ELSE},
    {"except", TOKEN_EXCEPT},
    {"exec", TOKEN_EXEC},
    {"finally", TOKEN_FINALLY},
    {"for", TOKEN_FOR},
    {"from", TOKEN_FROM},
    {"global", TOKEN_GLOBAL},
    {"if", TOKEN_IF},
    {"import", TOKEN_IMPORT},
    {"in", TOKEN_IN},
    {"is", TOKEN_IS},
    {"lambda", TOKEN_LAMBDA},
    {"local", TOKEN_LOCAL},
    {"nonlocal", TOKEN_NONLOCAL},
    {"not", TOKEN_NOT},
    {"or", TOKEN_OR},
    {"pass", TOKEN_PASS},
    {"raise", TOKEN_RAISE},
    {"return", TOKEN_RETURN},
    {"try", TOKEN_TRY},
    {"while", TOKEN_WHILE},
    {"with", TOKEN_WITH},
    {"yield", TOKEN_YIELD},
    {"False", TOKEN_FALSE},
    {"None", TOKEN_NONE},
    {"True", TOKEN_TRUE},
};

Token *Lexer::ProcessLineContinuation()
{
    string lex;
    lex += Get(); // backslash

    // Allow trailing whitespace only if followed by a comment.
    bool trailingSpace = false;
    int c;
    while (c = Peek(), isspace(c) && c != '\n')
    {
        trailingSpace = true;
        lex += Get();
    }
    if (c == '#')
    {
        trailingSpace = false;
        ProcessComment();
    }
    Get(); // newline

    return !trailingSpace ? 0 : new Token(sourceLocation, -1, lex);
}

Token *Lexer::ProcessComment()
{
    int c;
    while (c = Peek(), c != '\n' && c != EOF)
        Get();
    return 0; // no token
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
                else if (c != '-')
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
            errno = 0;
            auto value = strtol(s, 0, 10);
            return
                errno != 0 || value < INT32_MIN || value > INT32_MAX ?
                new Token
                    (sourceLocation, -1, lex,
                     "Decimal constant out of range") :
                new Token
                    (sourceLocation, static_cast<int>(value), 0, lex);
        }

        case State::Hexadecimal:
        {
            bool negative = *s == '-';
            if (negative)
                s++;
            errno = 0;
            unsigned long ulValue = strtoul(s, 0, 0x10);
            bool error = errno != 0 || ulValue > UINT32_MAX;
            uint32_t uValue = static_cast<uint32_t>(ulValue);
            int32_t value = *reinterpret_cast<int32_t *>(&uValue);
            if (!error && negative)
            {
                error = value == INT32_MIN;
                if (!error)
                    value = -value;
            }
            return error ?
                new Token
                    (sourceLocation, -1, lex,
                     "Hexadecimal constant out of range") :
                new Token
                    (sourceLocation, static_cast<int>(value), 0, lex);
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
    for (n -= static_cast<unsigned>(prefetch.size()), n++;
         !is.eof() && n > 0; n--)
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

    // Ensure the last character ends a line.
    if (c == EOF)
    {
        readahead.push_back(c);
        c = '\n';
    }

    return c;
}
