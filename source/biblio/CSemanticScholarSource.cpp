#include "biblio/CSemanticScholarSource.h"
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#pragma comment(lib, "winhttp.lib")

std::vector<CArticle> CSemanticScholarSource::GetArticles(const std::string& query, int start)
{
    std::wstring wQuery;
    for (char c : query)
    {
        if (c == ' ')       wQuery += L"%20";
        else if (c == '&')  wQuery += L"%26";
        else if (c == '+')  wQuery += L"%2B";
        else if (c == '#')  wQuery += L"%23";
        else                wQuery += static_cast<wchar_t>(c);
    }

    std::wstring wOffset = std::to_wstring(start);
    std::wstring path = L"/graph/v1/paper/search?query=" + wQuery
        + L"&limit=50&offset=" + wOffset
        + L"&fields=paperId,title,authors,abstract,year,externalIds,openAccessPdf";

    std::string json = HttpGet(L"api.semanticscholar.org", path);
    if (json.empty()) return {};

    std::vector<CArticle> articles;
    ParseResponse(articles, json);
    return articles;
}

// Find matching closing brace starting at the '{' at objStart.
// Handles strings (with escapes) so braces inside strings are ignored.
static std::size_t FindClosingBrace(const std::string& s, std::size_t objStart)
{
    int depth = 0;
    bool inString = false;
    for (std::size_t i = objStart; i < s.size(); i++)
    {
        if (inString)
        {
            if (s[i] == '\\') { i++; continue; } // skip escaped char
            if (s[i] == '"')  inString = false;
        }
        else
        {
            if      (s[i] == '"') inString = true;
            else if (s[i] == '{') depth++;
            else if (s[i] == '}') { if (--depth == 0) return i + 1; }
        }
    }
    return s.size();
}

void CSemanticScholarSource::ParseResponse(std::vector<CArticle>& articles, const std::string& json)
{
    // Locate "data" array so we don't accidentally parse the outer wrapper object
    auto dataPos = json.find("\"data\"");
    if (dataPos == std::string::npos) return;

    std::size_t searchFrom = dataPos;

    while (true)
    {
        // Find next "paperId" key
        auto pidKey = json.find("\"paperId\"", searchFrom);
        if (pidKey == std::string::npos) break;

        // Walk backward from pidKey to find the opening '{' of this paper object
        std::size_t objStart = pidKey;
        while (objStart > 0 && json[objStart] != '{') objStart--;
        if (json[objStart] != '{') break;

        // Find the matching closing '}' using brace counting (ignores braces in strings)
        std::size_t objEnd = FindClosingBrace(json, objStart);

        std::string entry = json.substr(objStart, objEnd - objStart);

        CArticle article;
        article.title    = ExtractJsonField(entry, "title");
        article.abstract = ExtractJsonField(entry, "abstract");

        std::string year = ExtractJsonField(entry, "year");
        article.published = year;

        std::string paperId = ExtractJsonField(entry, "paperId");
        article.url = "https://www.semanticscholar.org/paper/" + paperId;

        // DOI lives inside "externalIds": {...}
        auto exStart = entry.find("\"externalIds\"");
        if (exStart != std::string::npos)
        {
            auto bracePos = entry.find('{', exStart);
            if (bracePos != std::string::npos)
            {
                auto exEnd = FindClosingBrace(entry, bracePos);
                std::string exObj = entry.substr(bracePos, exEnd - bracePos);
                article.doi = ExtractJsonField(exObj, "DOI");
            }
        }

        // Authors: parse "authors":[{"authorId":...,"name":"..."}]
        auto authPos = entry.find("\"authors\"");
        if (authPos != std::string::npos)
        {
            auto bracketStart = entry.find('[', authPos);
            auto bracketEnd   = entry.find(']', bracketStart);
            if (bracketStart != std::string::npos && bracketEnd != std::string::npos)
            {
                std::string authArray = entry.substr(bracketStart, bracketEnd - bracketStart);
                std::size_t aPos = 0;
                while (true)
                {
                    auto nameKey = authArray.find("\"name\"", aPos);
                    if (nameKey == std::string::npos) break;
                    std::string name = ExtractJsonField(authArray.substr(nameKey), "name");
                    if (!name.empty()) article.authors.push_back(name);
                    aPos = nameKey + 6;
                }
            }
        }

        if (!article.title.empty())
            articles.push_back(article);

        searchFrom = objEnd;
    }
}

std::string CSemanticScholarSource::ExtractJsonField(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;

    // skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                  json[pos] == '\r' || json[pos] == '\n')) pos++;
    if (pos >= json.size()) return "";

    // null
    if (json.size() >= pos + 4 && json.substr(pos, 4) == "null") return "";

    // string value
    if (json[pos] == '"')
    {
        pos++;
        std::string result;
        while (pos < json.size() && json[pos] != '"')
        {
            if (json[pos] == '\\' && pos + 1 < json.size())
            {
                pos++;
                switch (json[pos])
                {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += json[pos]; break;
                }
            }
            else
            {
                result += json[pos];
            }
            pos++;
        }
        return result;
    }

    // number value
    if ((json[pos] >= '0' && json[pos] <= '9') || json[pos] == '-')
    {
        auto end = pos;
        while (end < json.size() && (json[end] >= '0' && json[end] <= '9')) end++;
        return json.substr(pos, end - pos);
    }

    return "";
}

std::string CSemanticScholarSource::HttpGet(const std::wstring& host, const std::wstring& path)
{
    auto closeAll = [](HINTERNET req, HINTERNET con, HINTERNET ses)
    {
        if (req) WinHttpCloseHandle(req);
        if (con) WinHttpCloseHandle(con);
        if (ses) WinHttpCloseHandle(ses);
    };

    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    WinHttpSetTimeouts(hSession, 15000, 15000, 20000, 60000);

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { closeAll(nullptr, nullptr, hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { closeAll(nullptr, hConnect, hSession); return {}; }

    // Disable cert revocation check — avoids failures on some corporate networks
    DWORD secFlags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                     SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                     SECURITY_FLAG_IGNORE_UNKNOWN_CA;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));

    if (!m_apiKey.empty())
    {
        std::wstring keyHeader = L"x-api-key: "
            + std::wstring(m_apiKey.begin(), m_apiKey.end()) + L"\r\n";
        WinHttpAddRequestHeaders(hRequest, keyHeader.c_str(), (ULONG)-1L,
            WINHTTP_ADDREQ_FLAG_ADD);
    }

    WinHttpAddRequestHeaders(hRequest,
        L"Accept: application/json\r\nConnection: close\r\n",
        (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        closeAll(hRequest, hConnect, hSession);
        return {};
    }

    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        closeAll(hRequest, hConnect, hSession);
        return {};
    }

    // Read response body regardless of status
    std::string response;
    DWORD dwSize = 0, dwDownloaded = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
        std::vector<char> buffer(dwSize + 1, 0);
        WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
        if (dwDownloaded > 0)
            response.append(buffer.data(), dwDownloaded);
    } while (dwDownloaded > 0);

    // Get status for logging
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    // Write debug log next to the executable
    {
        std::ofstream log("semantic_scholar.log", std::ios::app);
        log << "--- Request ---\n";
        log << "Status: " << statusCode << "\n";
        log << "Body (" << response.size() << " bytes): "
            << response.substr(0, std::min(response.size(), std::size_t(800))) << "\n\n";
    }

    closeAll(hRequest, hConnect, hSession);

    // Reject rate-limit / server-error responses (body has no useful data)
    if (statusCode == 429 || statusCode >= 500)
        return {};

    return response;
}
