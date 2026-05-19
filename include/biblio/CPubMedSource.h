#pragma once
#include "biblio/CArticleSource.h"

class CPubMedSource : public CArticleSource 
{
public:
    std::vector<CArticle> GetArticles(const std::string& query, int start = 0) override;

private:
    std::vector<std::string> FetchIDs(const std::string& query, int start);
    std::string FetchDetails(const std::vector<std::string>& ids);
    void ParseResponse(std::vector<CArticle>& articles, const std::string& xml);

    std::string HttpGet(const std::wstring& host, const std::wstring& path);
    std::string ExtractTag(const std::string& xml, const std::string& tag);
};