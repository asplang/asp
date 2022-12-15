//
// Asp grammar definitions.
//

#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

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
