//
// Asp application specification statement implementation.
//

#include "statement.hpp"

using namespace std;

Assignment::Assignment(const Token &nameToken, Literal *value) :
    NonTerminal(nameToken),
    value(value)
{
}

Assignment::~Assignment()
{
    delete value;
}

Parameter::Parameter(const Token &nameToken, Literal *defaultValue) :
    NonTerminal(nameToken),
    name(nameToken.s),
    type(Type::Positional),
    defaultValue(defaultValue)
{
}

Parameter::Parameter(const Token &nameToken, Type type) :
    NonTerminal(nameToken),
    name(nameToken.s),
    type(type),
    defaultValue(nullptr)
{
}

Parameter::~Parameter()
{
    delete defaultValue;
}

void ParameterList::Add(Parameter *parameter)
{
    if (parameters.empty())
        (SourceElement &)*this = *parameter;
    parameters.push_back(parameter);
}

ParameterList::~ParameterList()
{
    for (auto &parameter: parameters)
        delete parameter;
}

FunctionDefinition::FunctionDefinition
    (const Token &nameToken, bool isLibraryInterface,
     const Token &internalNameToken, ParameterList *parameterList) :
    NonTerminal(nameToken),
    isLibraryInterface(isLibraryInterface),
    internalName(internalNameToken.s),
    parameterList(parameterList)
{
}

FunctionDefinition::~FunctionDefinition()
{
    delete parameterList;
}

#ifdef ASP_FEATURE_CLASS

ClassDefinition::ClassDefinition(const Token &nameToken) :
    NonTerminal(nameToken),
    definitions(new DefinitionMap)
{
}

#endif

void NameList::Add(const Token &nameToken)
{
    if (names.empty())
        (SourceElement &)*this = nameToken;
    names.push_back(nameToken.s);
}
