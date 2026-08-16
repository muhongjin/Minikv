#include "utils.h"

 std::vector<std::string> split(const std::string& str, char delim){
    std::vector<std::string> res;
    std::istringstream ss(str);
    std::string item;
    while(std::getline(ss, item, delim)){
        if(!item.empty()){
            res.push_back(item);
        }
    }
    return res;
 }