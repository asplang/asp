//
// Asp application specification statement definitions.
//

#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include "literal.hpp"
#include "grammar.hpp"
#include "token.h"
#include <list>
#include <string>

class Assignment : public NonTerminal
{
    public:

        Assignment(const Token &name, Literal *);
        ~Assignment();

        std::string Name() const
        {
            return name;
        }
        Literal *Value() const
        {
            return value;
        }

    private:

        std::string name;
        Literal *value;
};

class Parameter : public NonTerminal
{
    public:

        enum class Type
        {
            Positional,
            TupleGroup,
            DictionaryGroup,
        };

        Parameter(const Token &name, Literal *);
        Parameter(const Token &name, Type = Type::Positional);
        ~Parameter();

        std::string Name() const
        {
            return name;
        }
        Type GetType() const
        {
            return type;
        }
        bool IsGroup() const
        {
            return IsTupleGroup() || IsDictionaryGroup();
        }
        bool IsTupleGroup() const
        {
            return type == Type::TupleGroup;
        }
        bool IsDictionaryGroup() const
        {
            return type == Type::DictionaryGroup;
        }
        Literal *DefaultValue() const
        {
            return defaultValue;
        }

    private:

        std::string name;
        Type type;
        Literal *defaultValue;
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

class FunctionDefinition : public NonTerminal
{
    public:

        FunctionDefinition
            (const Token &name, bool isLibraryInterface,
             const Token &internalName, ParameterList *);
        ~FunctionDefinition();

        const std::string &Name() const
        {
            return name;
        }
        bool IsLibraryInterface() const
        {
            return isLibraryInterface;
        }
        const std::string &InternalName() const
        {
            return internalName;
        }
        const ParameterList &Parameters() const
        {
            return *parameterList;
        }

    private:

        std::string name;
        bool isLibraryInterface;
        std::string internalName;
        ParameterList *parameterList;
};

class NameList : public NonTerminal
{
    public:

        void Add(Token &name);

        typedef std::list<std::string>::const_iterator ConstNameIterator;
        ConstNameIterator NamesBegin() const
        {
            return names.begin();
        }
        ConstNameIterator NamesEnd() const
        {
            return names.end();
        }

    private:

        std::list<std::string> names;
};

#endif
