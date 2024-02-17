//
// Asp function checking mplementation.
//

#include "function.hpp"
#include <sstream>

using namespace std;

string ValidFunctionDefinition::AddParameter
    (const string &name, ParameterType type, bool hasDefault)
{
    try
    {
        AddParameter1(name, type, hasDefault);
        return "";
    }
    catch (const string &error)
    {
        valid = false;
        return error;
    }
}

void ValidFunctionDefinition::AddParameter1
    (const string &name, ParameterType type, bool hasDefault)
{
    if (!valid)
        throw string("Internal error checking function definition");

    // Ensure parameter name is not duplicated.
    unsigned prevPosition = 1;
    for (auto prevIter = names.begin();
         prevIter != names.end(); prevIter++, prevPosition++)
    {
        const auto &prevName = *prevIter;
        if (prevName == name)
        {
            ostringstream oss;
            oss
                << "Duplicate parameter name '" << name
                << "' (" << (names.size() + 1) << " vs. "
                << prevPosition << ')';
            throw oss.str();
        }
    }
    names.push_back(name);

    // Ensure the dictionary group parameter, if present, is the last
    // parameter.
    if (dictionaryGroupSeen)
    {
        ostringstream oss;
        oss
            << "Parameter '" << name << "' (" << names.size()
            << ") follows dictionary group parameter";
        throw oss.str();
    }

    // Ensure there is only one tuple group parameter.
    if (type == ParameterType::TupleGroup)
    {
        if (tupleGroupSeen)
        {
            ostringstream oss;
            oss
                << "Duplicate tuple group parameter '" << name
                << "' (" << names.size() << ")";
            throw oss.str();
        }
        tupleGroupSeen = true;
    }

    if (type == ParameterType::DictionaryGroup)
        dictionaryGroupSeen = true;
    else if (hasDefault)
        defaultSeen = true;
    else
    {
        // Prior to any tuple group parameter, ensure that parameters with
        // defaults are not followed by parameters without.
        if (defaultSeen && !tupleGroupSeen)
        {
            ostringstream oss;
            oss
                << "Parameter '" << name << "' (" << names.size()
                << ") with no default value"
                << " follows parameter(s) with default value(s)";
            throw oss.str();
        }
    }
}
