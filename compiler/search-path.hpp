//
// Asp search path definitions.
//

#ifndef SEARCH_PATH_HPP
#define SEARCH_PATH_HPP

#include <vector>
#include <string>

class SearchPath : public std::vector<std::string>
{
    public:

        // Constructor.
        explicit SearchPath(const std::string &pathString = "");
};

#endif
