#pragma once

#include <string>
#include <vector>    

struct CArticle
{
    std::string title;
    std::vector<std::string> authors;
    std::string journal;
    std::string volume;
    std::string number;
    std::string published;
    std::string url;
    std::string doi;
    std::string abstract;
};