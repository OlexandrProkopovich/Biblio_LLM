#pragma once
#include "biblio/CArticleSource.h"
#include <string>

class CSemanticScholarSource : public CArticleSource
{
public:
    explicit CSemanticScholarSource(const std::string& apiKey = "") : m_apiKey(apiKey) {}

    std::vector<CArticle> GetArticles(const std::string& query, int start = 0) override;

private:
    std::string ExtractJsonField(const std::string& json, const std::string& key);
    std::string HttpGet(const std::wstring& host, const std::wstring& path);
    void ParseResponse(std::vector<CArticle>& articles, const std::string& json);

    std::string m_apiKey;
};