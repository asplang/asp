//
// Asp search path implementation.
//

#include "search-path.hpp"

#ifndef PATH_NAME_SEPARATOR
#error PATH_NAME_SEPARATOR macro undefined
#endif

using namespace std;

SearchPath::SearchPath(const string &searchPathString)
{
    emplace_back();

    if (searchPathString.empty())
        return;

    for (const char *p = searchPathString.c_str(); *p != '\0'; p++)
    {
        char c = *p;
        if (c == PATH_NAME_SEPARATOR)
            emplace_back();
        else
            back() += c;
    }
}
