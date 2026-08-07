#include "utils.h"

#include <sstream>

std::vector<std::string> split(const std::string& str, char delim)
{
    std::vector<std::string> result;
    std::istringstream input(str);
    std::string item;
    while (std::getline(input, item, delim)) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}
