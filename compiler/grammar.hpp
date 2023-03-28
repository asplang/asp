//
// Asp grammar definitions.
//

#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <string>

struct SourceLocation
{
    SourceLocation() :
        line(0), column(0)
    {
    }

    SourceLocation
        (const std::string &fileName,
         unsigned line, unsigned column) :
        fileName(fileName),
        line(line), column(column)
    {
    }

    std::string fileName;
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

    bool HasSourceLocation() const
    {
        return sourceLocation.line != 0;
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
