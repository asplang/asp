//
// Asp grammar definitions.
//

#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <string>
#include <utility>

class SourceLocation
{
    public:

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

        bool Defined() const
        {
            return !fileName.empty();
        }

        std::string fileName;
        unsigned line, column;
};

class SourceElement
{
    public:

        SourceElement() = default;

        explicit SourceElement(const SourceLocation &sourceLocation) :
            sourceLocation(sourceLocation)
        {
        }

        bool HasSourceLocation() const
        {
            return sourceLocation.line != 0;
        }

        [[noreturn]]
        void ThrowError(const std::string &error) const
        {
            throw make_pair(*this, error);
        }

        SourceLocation sourceLocation;
};

class NonTerminal : public SourceElement
{
    public:

        // Constructor, destructor.
        explicit NonTerminal(const SourceElement & = SourceElement());
        virtual ~NonTerminal() = default;

    protected:

        // Copy prevention.
        NonTerminal(const NonTerminal &) = delete;
        NonTerminal operator =(const NonTerminal &) = delete;
};

#endif
