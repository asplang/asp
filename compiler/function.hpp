//
// Asp function checking definitions.
//

#ifndef FUNCTION_HPP
#define FUNCTION_HPP

#include "statement.hpp"
#include <vector>
#include <string>

class ValidFunctionDefinition
{
    public:

        using ParameterType = Parameter::Type;
        std::string AddParameter
            (const std::string &name, ParameterType, bool hasDefault);
        bool IsValid() const
        {
            return valid;
        }

    protected:

        void AddParameter1
            (const std::string &name, ParameterType, bool hasDefault);

    private:

        std::vector<std::string> names;
        bool
            valid = true,
            defaultSeen = false,
            tupleGroupSeen = false, dictionaryGroupSeen = false;
};

#endif
