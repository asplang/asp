//
// Asp application specification statement implementation.
//

#include "statement.hpp"

using namespace std;

Parameter::Parameter(const Token &nameToken, bool isGroup) :
    NonTerminal(nameToken),
    name(nameToken.s),
    isGroup(isGroup)
{
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

bool FunctionDefinition::operator <(const FunctionDefinition &right) const
{
    return name < right.name;
}
