#include "core/tokenizer/CTokenizer.h"
#include <fstream>
#include <cctype>

static std::string normalizeWord(const std::string& word)
{
    std::string result;
    for (unsigned char c : word)
        if (std::isalpha(c))
            result += static_cast<char>(std::tolower(c));
    return result;
}

void CTokenizer::Build(const std::string& text)
{
    std::size_t index = m_wordToIndex.size();
    std::string raw;

    for (std::size_t i = 0; i <= text.size(); i++)
    {
        char c = (i < text.size()) ? text[i] : ' ';
        if (c != ' ')
        {
            raw += c;
        }
        else
        {
            std::string word = normalizeWord(raw);
            raw.clear();

            if (!word.empty() && m_wordToIndex.find(word) == m_wordToIndex.end())
            {
                m_wordToIndex[word] = index;
                m_indexToWord[index] = word;
                index++;
            }
        }
    }
}

std::vector<std::size_t> CTokenizer::Encode(const std::string& text) const
{
    std::vector<std::size_t> result;
    std::string raw;

    for (std::size_t i = 0; i <= text.size(); i++)
    {
        char c = (i < text.size()) ? text[i] : ' ';
        if (c != ' ')
        {
            raw += c;
        }
        else
        {
            std::string word = normalizeWord(raw);
            raw.clear();

            if (!word.empty())
            {
                auto it = m_wordToIndex.find(word);
                if (it != m_wordToIndex.end())
                    result.push_back(it->second);
            }
        }
    }

    return result;
}

std::string CTokenizer::Decode(const std::vector<std::size_t>& tokens) const
{
    std::string result;
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        auto it = m_indexToWord.find(tokens[i]);
        if (it == m_indexToWord.end() || it->second.empty())
            continue;
        if (i > 0) result += ' ';
        result += it->second;
    }
    return result;
}

std::size_t CTokenizer::VocabSize() const
{
    return m_wordToIndex.size();
}

void CTokenizer::Save(std::ofstream& file) const
{
    std::size_t size = m_wordToIndex.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& pair : m_wordToIndex)
    {
        std::size_t len = pair.first.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(pair.first.data(), len);
        file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
    }
}

void CTokenizer::Load(std::ifstream& file)
{
    std::size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    for (std::size_t i = 0; i < size; i++)
    {
        std::size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string word(len, '\0');
        file.read(word.data(), len);
        std::size_t idx;
        file.read(reinterpret_cast<char*>(&idx), sizeof(idx));
        m_wordToIndex[word] = idx;
        m_indexToWord[idx] = word;
    }
}
