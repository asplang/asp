/*
 * Asp application specification lexical analyzer definitions.
 */

#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#ifdef __cplusplus
#include <iostream>
#include <deque>
#include <map>
#include <string>
#include <cstdint>

extern "C" {

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
        Token *ProcessLineContinuation();
        Token *ProcessComment();
        Token *ProcessStatementEnd();
        Token *ProcessNumber();
        Token *ProcessSignedNumber();
        Token *ProcessString();
        Token *ProcessName();
        Token *ProcessSpecial();

        // Character methods.
        int Get();
        int Peek(unsigned offset = 0);
        int Read();

    private:

        // Constants.
        static bool keywordsInitialized;
        static std::map<std::string, int> keywords;

        // Data.
        std::istream &is;
        std::deque<int> prefetch;
        std::deque<int> readahead;
        SourceLocation sourceLocation, caret = {1, 1};
};

} // extern "C"

#endif

#endif
