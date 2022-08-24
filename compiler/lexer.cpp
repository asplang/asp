//
// Asp lexical analyzer implementation.
//

#include "lexer.h"
#include "asp.h"
#include <cctype>
#include <cstring>
#include <map>
#include <algorithm>
#include <limits>

using namespace std;

static bool IsSpecial(int);

const int CHAR_LINE_CONTINUATION = 0x100;

Lexer::Lexer(istream &is) :
    is(is)
{
}

Token *Lexer::Next()
{
    if (pendingTokens.empty())
        FetchNext();
    auto token = pendingTokens.front();
    pendingTokens.pop_front();

    // Look ahead if necessary.
    if ((token->type == TOKEN_IS || token->type == TOKEN_NOT) &&
        pendingTokens.empty())
        FetchNext();

    Token *nextToken = 0;

    // Turn "is" followed by "not" into "is not".
    if (token->type == TOKEN_IS &&
        (nextToken = pendingTokens.front())->type == TOKEN_NOT)
    {
        token->type = TOKEN_IS_NOT;
        token->s += ' ';
        token->s += nextToken->s;
        pendingTokens.pop_front();
        delete nextToken;
    }

    // Turn "not" followed by "in" into "not in".
    if (token->type == TOKEN_NOT &&
        (nextToken = pendingTokens.front())->type == TOKEN_IN)
    {
        token->type = TOKEN_NOT_IN;
        token->s += ' ';
        token->s += nextToken->s;
        pendingTokens.pop_front();
        delete nextToken;
    }

    return token;
}

void Lexer::FetchNext()
{
    while (pendingTokens.empty())
    {
        sourceLocation = caret;

        // Check indent if applicable. Note that this may generate pending
        // tokens to indicate block-end or indent errors.
        int c = Peek();
        if (c == CHAR_LINE_CONTINUATION)
            continueLine = true;
        else
        {
             if (c == '\n' && !continueLine)
                checkIndent = true;
             continueLine = false;
        }
        if (checkIndent && !isspace(c) && c != '\n')
            CheckIndent();
        if (!pendingTokens.empty())
            continue;

        Token *token = 0;
        if (c == EOF)
            token = new Token(sourceLocation);
        else if (c == '\n' || c == ';')
            token = ProcessStatementEnd();
        else if (isdigit(c))
            token = ProcessNumber();
        else if (isalpha(c) || c == '_')
            token = ProcessName();
        else if (c == '\'' || c == '"')
            token = ProcessString();
        else if (c == '.')
            token = isdigit(Peek(1)) ? ProcessNumber() : ProcessSpecial();
        else if (c == ':')
            token = Peek(1) == '\n' ? ProcessIndent() : ProcessSpecial();
        else if (IsSpecial(c))
            token = ProcessSpecial();
        else
        {
            Get();
            if (!isspace(c) && c != CHAR_LINE_CONTINUATION)
                token = new Token(sourceLocation, -1, string(1, c));
        }

        if (token)
            pendingTokens.push_back(token);
    }
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
    int type;
    auto iter = keywords.find(lex);
    return
        iter != keywords.end() ?
        new Token(sourceLocation, iter->second, lex) :
        new Token(sourceLocation, TOKEN_NAME, lex);
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

Token *Lexer::ProcessSpecial()
{
    int c = Get();
    int c2 = Peek();

    string lex;
    lex += static_cast<char>(c);

    static const char dbl[] = "*/<>.=";
    static const char eq2[] = "+-*/%<>=&^|";
    static const char eq3[] = "*/<>";
    int type = -1;
    if (c == '!')
    {
        if (c2 != '=')
            return new Token(sourceLocation, -1, lex);
        lex += static_cast<char>(Get());
    }
    else if (c == '.' && c2 == '.' && Peek(1) == '.')
    {
        lex += static_cast<char>(Get());
        lex += static_cast<char>(Get());
    }
    else if (c == '<' && c2 == '-')
    {
        lex += static_cast<char>(Get());
    }
    else if (strchr(dbl, c) != 0 && c2 == c)
    {
        lex += static_cast<char>(Get());
        if (strchr(eq3, c) != 0 && Peek() == '=')
            lex += static_cast<char>(Get());
    }
    else if (strchr(eq2, c) != 0 && c2 == '=')
    {
        lex += static_cast<char>(Get());
    }

    static const map<string, int> keywords =
    {
        {",", TOKEN_COMMA},
        {"+", TOKEN_PLUS},
        {"-", TOKEN_MINUS},
        {"*", TOKEN_ASTERISK},
        {"/", TOKEN_SLASH},
        {"%", TOKEN_PERCENT},
        {"<", TOKEN_LT},
        {">", TOKEN_GT},
        {"=", TOKEN_ASSIGN},
        {"&", TOKEN_AMPERSAND},
        {"^", TOKEN_CARET},
        {"|", TOKEN_BAR},
        {".", TOKEN_PERIOD},
        {":", TOKEN_COLON},
        {"`", TOKEN_BACK_QUOTE},
        {"(", TOKEN_LEFT_PAREN},
        {")", TOKEN_RIGHT_PAREN},
        {"[", TOKEN_LEFT_BRACKET},
        {"]", TOKEN_RIGHT_BRACKET},
        {"{", TOKEN_LEFT_BRACE},
        {"}", TOKEN_RIGHT_BRACE},
        {"~", TOKEN_TILDE},
        {"**", TOKEN_POWER},
        {"//", TOKEN_FLOOR_DIVIDE},
        {"<<", TOKEN_LEFT_SHIFT},
        {">>", TOKEN_RIGHT_SHIFT},
        {"..", TOKEN_RANGE},
        {"+=", TOKEN_PLUS_ASSIGN},
        {"-=", TOKEN_MINUS_ASSIGN},
        {"*=", TOKEN_TIMES_ASSIGN},
        {"/=", TOKEN_DIVIDE_ASSIGN},
        {"%=", TOKEN_MODULO_ASSIGN},
        {"<-", TOKEN_INSERT},
        {"<=", TOKEN_LE},
        {">=", TOKEN_GE},
        {"==", TOKEN_EQ},
        {"&=", TOKEN_BIT_AND_ASSIGN},
        {"^=", TOKEN_BIT_XOR_ASSIGN},
        {"|=", TOKEN_BIT_OR_ASSIGN},
        {"!=", TOKEN_NE},
        {"**=", TOKEN_POWER_ASSIGN},
        {"//=", TOKEN_FLOOR_DIVIDE_ASSIGN},
        {"<<=", TOKEN_LEFT_SHIFT_ASSIGN},
        {">>=", TOKEN_RIGHT_SHIFT_ASSIGN},
        {"...", TOKEN_ELLIPSIS},
    };
    if (type == -1)
    {
        auto iter = keywords.find(lex);
        if (iter != keywords.end())
            type = iter->second;
    }
    return new Token(sourceLocation, type, lex);
}

Token *Lexer::ProcessIndent()
{
    Get(); // :
    Get(); // \n
    expectIndent = true;
    checkIndent = true;
    return new Token(sourceLocation, TOKEN_BLOCK_START);
}

int Lexer::Get()
{
    // Get the next character from the peek prefetch or the file as appropriate.
    int c;
    if (!prefetch.empty())
    {
        c = prefetch.front();
        prefetch.pop_front();
    }
    else
        c = Read();

    // Maintain indent level.
    if (checkIndent && isspace(c) && c != '\n')
        currIndent += c;

    // Maintain line/column.
    if (c == '\n' || c == CHAR_LINE_CONTINUATION)
    {
        caret.column = 0;
        caret.line++;
        currIndent.clear();
    }
    caret.column++;

    return c;
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

void Lexer::CheckIndent()
{
    checkIndent = false;
    auto minSize = min(currIndent.size(), prevIndent.size());
    bool consistent =
        currIndent.substr(0, minSize) == prevIndent.substr(0, minSize);
    if (!consistent)
    {
        pendingTokens.push_back
            (new Token(sourceLocation, TOKEN_INCONSISTENT_WS));
    }
    else if (expectIndent)
    {
        // Ensure expected indent is present.
        expectIndent = false;
        if (currIndent.size() > prevIndent.size())
            indents.push_back(currIndent.size() - prevIndent.size());
        else
            pendingTokens.push_back
                (new Token(sourceLocation, TOKEN_MISSING_INDENT));
    }
    else if (currIndent.size() < prevIndent.size())
    {
        // Determine matching shallower indent level.
        auto unindentSize = prevIndent.size();
        int unindentCount = 0;
        while (unindentSize > currIndent.size() && !indents.empty())
        {
            auto indentSize = indents.back();
            indents.pop_back();
            unindentSize -= indentSize;
            unindentCount++;
        }
        if (unindentSize == currIndent.size())
        {
            while (unindentCount--)
                pendingTokens.push_back
                    (new Token(sourceLocation, TOKEN_BLOCK_END));
        }
        else
            pendingTokens.push_back
                (new Token(sourceLocation, TOKEN_MISMATCHED_UNINDENT));
    }
    else if (currIndent.size() > prevIndent.size())
    {
        pendingTokens.push_back
            (new Token(sourceLocation, TOKEN_UNEXPECTED_INDENT));
    }

    prevIndent = currIndent;
}

Token::Token
    (const SourceLocation &sourceLocation, int type, const string &s) :
    SourceElement(sourceLocation),
    type(type),
    s(s)
{
}

Token::Token
    (const SourceLocation &sourceLocation, int value, int,
     const string &s) :
    SourceElement(sourceLocation),
    type(TOKEN_INTEGER),
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

static bool IsSpecial(int c)
{
    static const char chars[] = "!\"%&'()*+,-./:<=>[]^`{|}~";
    return c <= 0xFF && strchr(chars, c) != 0;
}
