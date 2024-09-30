//
// Asp lexical analyzer implementation.
//

#include "lexer.h"
#include "token-types.h"
#include <cctype>
#include <cstring>
#include <map>

using namespace std;

static bool IsSpecial(int);

bool Lexer::keywordsInitialized = true;

Lexer::Lexer(istream &is, const string &fileName) :
    is(is),
    caret(fileName, 1, 1)
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

    Token *nextToken = nullptr;

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
        Token *token = nullptr;

        // Check indent if applicable. Note that this may generate pending
        // tokens to indicate block-end or indent errors.
        int c = Peek();
        if (c == '\\')
        {
            token = ProcessLineContinuation();
            if (token)
                pendingTokens.push_back(token);
            else
                continueLine = true;
        }
        else
        {
             if (c == '\n' && !continueLine)
                checkIndent = true;
             continueLine = false;
        }
        if (checkIndent && !isspace(c) && c != '\n' && c != '#')
            CheckIndent();
        if (!pendingTokens.empty())
            continue;

        c = Peek();
        if (c == EOF)
            token = new Token(sourceLocation);
        else if (c == '#')
            token = ProcessComment();
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
        {
            int cx;
            int offset = 1;
            while (cx = Peek(offset), isspace(cx) && cx != '\n')
                offset++;
            token =
                isspace(cx) || cx == '#' ?
                ProcessIndent() : ProcessSpecial();
        }
        else if (IsSpecial(c))
            token = ProcessSpecial();
        else
        {
            Get();
            if (!isspace(c))
                token = new Token
                    (sourceLocation, -1, string(1, static_cast<char>(c)));
        }

        if (token)
            pendingTokens.push_back(token);
    }
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
    else if (c != '\0' && strchr(dbl, c) != nullptr && c2 == c)
    {
        lex += static_cast<char>(Get());
        if (strchr(eq3, c) != nullptr && Peek() == '=')
            lex += static_cast<char>(Get());
    }
    else if (c != '\0' && strchr(eq2, c) != nullptr && c2 == '=')
    {
        lex += static_cast<char>(Get());
        if (Peek() == '>')
            lex += static_cast<char>(Get());
    }

    static const map<string, int> operators =
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
        {"`", TOKEN_GRAVE},
        {"(", TOKEN_LEFT_PAREN},
        {")", TOKEN_RIGHT_PAREN},
        {"[", TOKEN_LEFT_BRACKET},
        {"]", TOKEN_RIGHT_BRACKET},
        {"{", TOKEN_LEFT_BRACE},
        {"}", TOKEN_RIGHT_BRACE},
        {"~", TOKEN_TILDE},
        {"**", TOKEN_DOUBLE_ASTERISK},
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
        {"<=>", TOKEN_ORDER},
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
        auto iter = operators.find(lex);
        if (iter != operators.end())
            type = iter->second;
    }
    return new Token(sourceLocation, type, lex);
}

Token *Lexer::ProcessIndent()
{
    Get(); // :

    // Allow trailing whitespace and/or a comment.
    int c;
    while (c = Peek(), isspace(c) && c != '\n')
        Get();
    if (c == '#')
        ProcessComment();
    Get(); // newline

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
        currIndent += static_cast<char>(c);

    // Maintain line/column.
    if (c == '\n')
    {
        caret.column = 0;
        caret.line++;
        currIndent.clear();
    }
    caret.column++;

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

static bool IsSpecial(int c)
{
    static const char chars[] = "!\"%&'()*+,-./:<=>[]^`{|}~";
    return c > 0 && c <= 0xFF && strchr(chars, c) != nullptr;
}
