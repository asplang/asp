/*
 * Asp lexical analyzer definitions.
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
        void FetchNext();
        Token *ProcessLineContinuation();
        Token *ProcessComment();
        Token *ProcessStatementEnd();
        Token *ProcessNumber();
        Token *ProcessString();
        Token *ProcessName();
        Token *ProcessSpecial();
        Token *ProcessIndent();

        // Character methods.
        int Get();
        int Peek(unsigned offset = 0);
        int Read();
        void CheckIndent();

    private:

        // Constants.
        static bool keywordsInitialized;
        static std::map<std::string, int> keywords;

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

} // extern "C"

#endif

#endif
