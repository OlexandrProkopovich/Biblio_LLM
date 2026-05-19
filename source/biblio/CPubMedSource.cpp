#include "biblio/CPubMedSource.h"
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

std::vector<CArticle> CPubMedSource::GetArticles(const std::string& query, int start)
{
    auto ids = FetchIDs(query, start);
    if (ids.empty()) 
        return {};

    std::string xml = FetchDetails(ids);
    std::vector<CArticle> articles;

    ParseResponse(articles, xml);

    return articles;
}

std::vector<std::string> CPubMedSource::FetchIDs(const std::string& query, int start)
{
    std::wstring wQuery(query.begin(), query.end());
    std::wstring wStart = std::to_wstring(start);
    std::wstring path = L"/entrez/eutils/esearch.fcgi?db=pubmed&term=" + wQuery + L"&retstart=" + wStart + L"&retmax=50&retmode=xml";

    std::string xml = HttpGet(L"eutils.ncbi.nlm.nih.gov", path);

    std::vector<std::string> ids;
    std::size_t pos = 0;
    while (true) 
    {
        auto start = xml.find("<Id>", pos);
        if (start == std::string::npos) 
            break;

        start += 4;
        auto end = xml.find("</Id>", start);

        ids.push_back(xml.substr(start, end - start));
        pos = end + 1;
    }
    return ids;
}

std::string CPubMedSource::FetchDetails(const std::vector<std::string>& ids) 
{
    std::string joined;
    for (std::size_t i = 0; i < ids.size(); i++) 
    {
        if (i > 0) 
            joined += ",";

        joined += ids[i];
    }
    std::wstring wIds(joined.begin(), joined.end());
    std::wstring path = L"/entrez/eutils/efetch.fcgi?db=pubmed&id=" + wIds + L"&retmode=xml";

    return HttpGet(L"eutils.ncbi.nlm.nih.gov", path);
}

void CPubMedSource::ParseResponse(std::vector<CArticle>& articles, const std::string& xml)
{
    std::size_t pos = 0;
    while (true)
    {
        auto start = xml.find("<PubmedArticle>", pos);
        if (start == std::string::npos) 
            break;

        auto end = xml.find("</PubmedArticle>", start);
        std::string entry = xml.substr(start, end - start);

        CArticle article;
        article.title = ExtractTag(entry, "ArticleTitle");
        article.abstract = ExtractTag(entry, "AbstractText");
        article.journal = ExtractTag(entry, "Title");
        article.published = ExtractTag(entry, "Year");

        std::size_t aPos = 0;
        while (true) 
        {
            auto aStart = entry.find("<Author ", aPos);
            if (aStart == std::string::npos)
                aStart = entry.find("<Author>", aPos);

            if (aStart == std::string::npos) 
                break;

            auto aEnd = entry.find("</Author>", aStart);
            std::string authorBlock = entry.substr(aStart, aEnd - aStart);
            std::string name = ExtractTag(authorBlock, "LastName") + " " + ExtractTag(authorBlock, "ForeName");

            if (!name.empty()) 
                article.authors.push_back(name);

            aPos = aEnd + 1;
        }

        std::string pmid = ExtractTag(entry, "PMID");
        article.url = "https://pubmed.ncbi.nlm.nih.gov/" + pmid;

        articles.push_back(article);
        pos = end + 1;
    }
}

std::string CPubMedSource::HttpGet(const std::wstring& host, const std::wstring& path) 
{
    HINTERNET hSession = WinHttpOpen(L"LLM/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    WinHttpReceiveResponse(hRequest, NULL);

    std::string response;
    DWORD dwSize = 0, dwDownloaded = 0;
    do {
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

std::string CPubMedSource::ExtractTag(const std::string& xml, const std::string& tag)
{
    std::string open = "<" + tag;
    std::string close = "</" + tag + ">";

    auto start = xml.find(open);
    if (start == std::string::npos) 
        return "";

    auto contentStart = xml.find(">", start);
    if (contentStart == std::string::npos) 
        return "";
    contentStart++;

    auto end = xml.find(close, contentStart);
    if (end == std::string::npos) 
        return "";

    return xml.substr(contentStart, end - contentStart);
}