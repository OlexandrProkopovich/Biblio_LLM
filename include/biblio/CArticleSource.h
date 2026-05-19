#pragma once

#include <vector>
#include <string>

#include "biblio/CArticle.h"

class CArticleSource
{
public:
    virtual std::vector<CArticle> GetArticles(const std::string& query, int start = 0) = 0;
    virtual ~CArticleSource() = default;
};