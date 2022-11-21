//
// Asp application specification statement definitions.
//

#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "grammar.hpp"
#include <list>
#include <string>

class Parameter : public NonTerminal
{
    public:

        explicit Parameter(const Token &name, bool group = false);

        std::string Name() const
        {
            return name;
        }
        bool IsGroup() const
        {
            return isGroup;
        }

    private:

        std::string name;
        bool isGroup;
};

class ParameterList : public NonTerminal
{
    public:

        ParameterList();
        ~ParameterList();

        void Add(Parameter *);

        typedef std::list<Parameter *>::const_iterator ConstParameterIterator;
        bool ParametersEmpty() const
        {
            return parameters.empty();
        }
        std::size_t ParametersSize() const
        {
            return parameters.size();
        }
        ConstParameterIterator ParametersBegin() const
        {
            return parameters.begin();
        }
        ConstParameterIterator ParametersEnd() const
        {
            return parameters.end();
        }

    private:

        std::list<Parameter *> parameters;
};

struct FunctionDefinition : public NonTerminal
{
    public:

        FunctionDefinition
            (const Token &name,
             const Token &internalName,
             ParameterList *);
        ~FunctionDefinition();

        const std::string &Name() const
        {
            return name;
        }
        const std::string &InternalName() const
        {
            return internalName;
        }
        const ParameterList &Parameters() const
        {
            return *parameterList;
        }

        bool operator <(const FunctionDefinition &right) const;

    private:

        std::string name;
        std::string internalName;
        ParameterList *parameterList;
};


#endif
