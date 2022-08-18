//
// Asp application specification statement implementation.
//

#include "statement.hpp"

using namespace std;

Parameter::Parameter(const Token &nameToken) :
    name(nameToken.s)
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
    (const string &name, const string &internalName,
     ParameterList *parameterList) :
    name(name),
    internalName(internalName),
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
