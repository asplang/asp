/*
 * Asp lexical analyzer definitions.
 */

#ifndef LEXER_H
#define LEXER_H

#ifdef __cplusplus
#include <iostream>
#include <deque>
#include <string>
#include <cstdint>
#endif

#ifndef __cplusplus
typedef struct Token Token;
#else
extern "C" {

struct Token;

struct SourceLocation
{
    SourceLocation() :
        line(0), column(0)
    {
    }

    SourceLocation(unsigned line, unsigned column) :
        line(line), column(column)
    {
    }

    unsigned line, column;
};

struct SourceElement
{
    SourceElement()
    {
    }

    explicit SourceElement(const SourceLocation &sourceLocation) :
        sourceLocation(sourceLocation)
    {
    }

    SourceLocation sourceLocation;
};

class Lexer
{
    public:

        // Constructor.
        explicit Lexer(std::istream &);

        // Next token method.
        Token *Next();

    protected:

        // Copy prevention.
        Lexer(const Lexer &) = delete;
        Lexer &operator =(const Lexer &) = delete;

        // Token scanning methods.
        void FetchNext();
        Token *ProcessStatementEnd();
        Token *ProcessNumber();
        Token *ProcessName();
        Token *ProcessString();
        Token *ProcessSpecial();
        Token *ProcessIndent();

        // Character methods.
        int Get();
        int Peek(unsigned offset = 0);
        int Read();
        void CheckIndent();

    private:

        // Data.
        std::istream &is;
        std::deque<int> prefetch;
        std::deque<int> readahead;
        SourceLocation sourceLocation, caret = {1, 1};
        bool checkIndent = true, expectIndent = false, continueLine = false;
        std::deque<unsigned> indents;
        std::string currIndent, prevIndent;
        std::deque<Token *> pendingTokens;
};

struct Token : public SourceElement
{
    explicit Token
        (const SourceLocation &, int type = 0, const std::string & = "");
    Token(const SourceLocation &, int, int, const std::string & = "");
    Token(const SourceLocation &, double, const std::string & = "");
    Token(const SourceLocation &, const std::string &);

    int type;
    union
    {
        std::int32_t i;
        double f;
    };
    std::string s;
};

}

#endif

#endif
