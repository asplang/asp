/*
 * Asp token definitions.
 */

#ifndef TOKEN_H
#define TOKEN_H

#ifdef __cplusplus
#include "grammar.hpp"
#include <string>
#include <cstdint>
#endif

#ifndef __cplusplus
typedef struct Token Token;
#else

extern "C" {

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

} // extern "C"

#endif

#endif
