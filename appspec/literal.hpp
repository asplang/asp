//
// Asp literal definitions.
//

#ifndef LITERAL_HPP
#define LITERAL_HPP

#include "grammar.hpp"
#include "token.h"
#include <string>
#include <cstdint>

class Literal : public NonTerminal
{
    public:

        // Constructor.
        explicit Literal(const Token &);

        // Type enumeration.
        enum class Type
        {
            None,
            Ellipsis,
            Boolean,
            Integer,
            Float,
            String,
        };

        // Methods.
        Type GetType() const
        {
            return type;
        }
        bool BooleanValue() const
        {
            return b;
        }
        std::int32_t IntegerValue() const
        {
            return i;
        }
        double FloatValue() const
        {
            return f;
        }
        std::string StringValue() const
        {
            return s;
        }

    private:

        // Data.
        Type type;
        union
        {
            bool b;
            std::int32_t i;
            double f;
        };
        std::string s;
};

#endif