//
// Asp grammar definitions.
//

#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include "lexer.h"

class NonTerminal : public SourceElement
{
    public:

        // Constructor, destructor.
        explicit NonTerminal(const SourceElement & = SourceElement());
        virtual ~NonTerminal();

    protected:

        // Copy prevention.
        NonTerminal(const NonTerminal &) = delete;
        NonTerminal operator =(const NonTerminal &) = delete;
};

#endif
