//
// Asp application specification parameter definitions.
//

#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include "grammar.hpp"
#include <list>
#include <string>

class Parameter : public NonTerminal
{
    public:

        Parameter(const Token &name);

        std::string Name() const
        {
            return name;
        }

    private:

        std::string name;
};

class ParameterList : public NonTerminal
{
    public:

        ParameterList();
        ~ParameterList();

        void Add(Parameter *);

        typedef std::list<Parameter *>::const_iterator ConstParameterIterator;
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

#endif
