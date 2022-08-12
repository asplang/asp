//
// Asp application specification parameter implementation.
//

#include "parameter.hpp"

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
