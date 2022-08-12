/*
 * Asp application specification lexical analyzer definitions.
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

        explicit Lexer(std::istream &);

        Token *Next();

    protected:

        // Copy prevention.
        Lexer(const Lexer &) = delete;
        Lexer &operator =(const Lexer &) = delete;

        // Token scanning methods.
        Token *ProcessStatementEnd();
        Token *ProcessName();
        Token *ProcessSpecial();

        // Character methods.
        int Get();
        int Peek(unsigned offset = 0);
        int Read();

    private:

        std::istream &is;
        std::deque<int> prefetch;
        std::deque<int> readahead;
        SourceLocation sourceLocation, caret = {1, 1};
};

struct Token : public SourceElement
{
    explicit Token
        (const SourceLocation &, int type = 0, const std::string & = "");

    int type;
    std::string s;
};

} // extern "C"

#endif

#endif
