#pragma once
#include "biblio/CArticleSource.h"

class CArxiveSource : public CArticleSource
{
public:
	std::vector<CArticle> GetArticles(const std::string& query, int start = 0) override;

private:
	void ParseResponse(std::vector<CArticle>& articles, const std::string& response);

	std::string FetchFromAPI(const std::string& query, int start);
	std::string ExtractTag(const std::string& xml, const std::string& tag);
};