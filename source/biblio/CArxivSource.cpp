#include "biblio/CArxivSource.h"

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")
#include <iostream>
#include <thread>
#include <chrono>

std::vector<CArticle> CArxiveSource::GetArticles(const std::string& query, int start)
{
	std::vector<CArticle> articles;

	std::string response = FetchFromAPI(query, start);
	if (!response.empty())
		ParseResponse(articles, response);

	return articles;
}

std::string CArxiveSource::FetchFromAPI(const std::string& query, int start)
{
	HINTERNET hSession = WinHttpOpen(L"LLM/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	HINTERNET hConnect = WinHttpConnect(hSession,
		L"export.arxiv.org",
		INTERNET_DEFAULT_HTTP_PORT, 0);

	std::wstring wQuery(query.begin(), query.end());
	std::wstring wStart = std::to_wstring(start);
	std::wstring path = L"/api/query?search_query=all:" + wQuery + L"&start=" + wStart + L"&max_results=50";

	HINTERNET hRequest = WinHttpOpenRequest(hConnect,
		L"GET", path.c_str(),
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

	WinHttpSendRequest(hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

	WinHttpReceiveResponse(hRequest, NULL);

	std::string response;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;

	do 
	{
		WinHttpQueryDataAvailable(hRequest, &dwSize);
		std::vector<char> buffer(dwSize + 1, 0);
		WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
		response += buffer.data();
	} while (dwSize > 0);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);


	return response;
}

void CArxiveSource::ParseResponse(std::vector<CArticle>& articles, const std::string& response)
{
	std::size_t pos = 0;

	while (true)
	{
		auto start = response.find("<entry>", pos);
		if (start == std::string::npos)
			break;

		auto end = response.find("</entry>", start);
		if (end == std::string::npos)
			break;

		std::string entry = response.substr(start, end - start);

		CArticle article;
		article.title = ExtractTag(entry, "title");

		std::size_t authorPos = 0;
		while (true)
		{
			auto aStart = entry.find("<author>", authorPos);
			if (aStart == std::string::npos) break;

			auto aEnd = entry.find("</author>", aStart);
			std::string authorBlock = entry.substr(aStart, aEnd - aStart);

			article.authors.push_back(ExtractTag(authorBlock, "name"));
			authorPos = aEnd + 1;
		}

		article.journal = ExtractTag(entry, "journal_ref");
		article.volume = ExtractTag(entry, "volume");
		article.number = ExtractTag(entry, "number");
		article.published = ExtractTag(entry, "published");
		article.url = ExtractTag(entry, "id");
		article.doi = ExtractTag(entry, "doi");
		article.abstract = ExtractTag(entry, "summary");

		articles.push_back(article);
		pos = end + 1;
	}
}

std::string CArxiveSource::ExtractTag(const std::string& xml, const std::string& tag)
{
	std::string open = "<" + tag;   // no closing > — handles <tag attr="...">
	std::string close = "</" + tag + ">";

	auto start = xml.find(open);
	if (start == std::string::npos)
		return "";

	// skip past the full opening tag (may have attributes)
	auto contentStart = xml.find('>', start);
	if (contentStart == std::string::npos)
		return "";
	contentStart++;

	auto end = xml.find(close, contentStart);
	if (end == std::string::npos)
		return "";

	return xml.substr(contentStart, end - contentStart);
}