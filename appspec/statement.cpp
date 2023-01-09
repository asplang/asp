//
// Asp application specification statement implementation.
//

#include "statement.hpp"

using namespace std;

Assignment::Assignment
    (const Token &nameToken, Literal *value) :
    name(nameToken.s),
    value(value)
{
}

Assignment::~Assignment()
{
    delete value;
}

Parameter::Parameter
    (const Token &nameToken, Literal *defaultValue, bool isGroup) :
    NonTerminal(nameToken),
    name(nameToken.s),
    defaultValue(defaultValue),
    isGroup(isGroup)
{
    if (defaultValue != 0 && isGroup)
        throw string("Group parameter cannot have a default");
}

Parameter::~Parameter()
{
    delete defaultValue;
}

ParameterList::ParameterList()
{
}

void ParameterList::Add(Parameter *parameter)
{
    if (parameters.empty())
        (SourceElement &)*this = *parameter;
    parameters.push_back(parameter);
}

ParameterList::~ParameterList()
{
    for (auto iter = parameters.begin(); iter != parameters.end(); iter++)
        delete *iter;
}

FunctionDefinition::FunctionDefinition
    (const Token &nameToken, const Token &internalNameToken,
     ParameterList *parameterList) :
    NonTerminal(nameToken),
    name(nameToken.s),
    internalName(internalNameToken.s),
    parameterList(parameterList)
{
}

FunctionDefinition::~FunctionDefinition()
{
    delete parameterList;
}
