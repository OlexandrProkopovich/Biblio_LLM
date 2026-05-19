#include "core/tokenizer/CBPETokenizer.h"
#include <algorithm>
#include <cctype>

static std::string toLower(const std::string& s)
{
    std::string r;
    for (unsigned char c : s)
        r += static_cast<char>(std::tolower(c));
    return r;
}

void CBPETokenizer::Build(const std::string& text, std::size_t numMerges)
{
    // 1. Count word frequencies (alpha-only, lowercased)
    std::unordered_map<std::string, std::size_t> wordFreq;
    std::string cur;
    for (std::size_t i = 0; i <= text.size(); i++)
    {
        char c = i < text.size() ? text[i] : ' ';
        if (std::isalpha(static_cast<unsigned char>(c)))
            cur += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else if (!cur.empty())
        {
            wordFreq[cur]++;
            cur.clear();
        }
    }

    // 2. Represent each word as a char sequence + end-of-word marker
    using Seq = std::vector<std::string>;
    std::unordered_map<std::string, Seq> wordSeqs;
    for (const auto& [w, _] : wordFreq)
    {
        Seq s;
        for (unsigned char c : w)
            s.push_back(std::string(1, static_cast<char>(c)));
        s.push_back("</w>");
        wordSeqs[w] = std::move(s);
    }

    // 3. Seed the vocabulary with all single characters
    for (const auto& [w, seq] : wordSeqs)
        for (const auto& tok : seq)
            if (!m_vocab.count(tok))
                m_vocab[tok] = m_vocab.size();

    // 4. Iterative BPE merges
    for (std::size_t iter = 0; iter < numMerges; iter++)
    {
        // Count pair frequencies weighted by word frequency
        std::unordered_map<std::string, std::size_t> pairCount;
        for (const auto& [w, seq] : wordSeqs)
        {
            std::size_t freq = wordFreq.at(w);
            for (std::size_t i = 0; i + 1 < seq.size(); i++)
            {
                std::string key = seq[i] + '\x01' + seq[i + 1]; // '\x01' as separator (not in tokens)
                pairCount[key] += freq;
            }
        }
        if (pairCount.empty()) break;

        // Best pair
        auto best = std::max_element(pairCount.begin(), pairCount.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        if (best->second < 2) break; // no pair appears more than once

        auto sep = best->first.find('\x01');
        std::string left    = best->first.substr(0, sep);
        std::string right   = best->first.substr(sep + 1);
        std::string merged  = left + right;

        m_merges.push_back({left, right});
        if (!m_vocab.count(merged))
            m_vocab[merged] = m_vocab.size();

        // Apply the merge to all word sequences
        for (auto& [w, seq] : wordSeqs)
        {
            Seq next;
            next.reserve(seq.size());
            for (std::size_t i = 0; i < seq.size(); i++)
            {
                if (i + 1 < seq.size() && seq[i] == left && seq[i + 1] == right)
                {
                    next.push_back(merged);
                    i++; // consume both
                }
                else
                {
                    next.push_back(seq[i]);
                }
            }
            seq = std::move(next);
        }
    }
}

std::vector<std::string> CBPETokenizer::EncodeWord(const std::string& word) const
{
    if (word.empty()) return {};

    // Start as character sequence + </w>
    std::vector<std::string> seq;
    std::string low = toLower(word);
    for (unsigned char c : low)
        seq.push_back(std::string(1, static_cast<char>(c)));
    seq.push_back("</w>");

    // Apply each merge rule greedily in the order they were learned
    for (const auto& [left, right] : m_merges)
    {
        std::vector<std::string> next;
        next.reserve(seq.size());
        for (std::size_t i = 0; i < seq.size(); i++)
        {
            if (i + 1 < seq.size() && seq[i] == left && seq[i + 1] == right)
            {
                next.push_back(left + right);
                i++;
            }
            else
            {
                next.push_back(seq[i]);
            }
        }
        seq = std::move(next);
    }
    return seq;
}

std::vector<std::size_t> CBPETokenizer::Encode(const std::string& text) const
{
    std::vector<std::size_t> ids;
    std::string word;
    for (std::size_t i = 0; i <= text.size(); i++)
    {
        char c = i < text.size() ? text[i] : ' ';
        if (std::isalpha(static_cast<unsigned char>(c)))
            word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else if (!word.empty())
        {
            for (const auto& tok : EncodeWord(word))
            {
                auto it = m_vocab.find(tok);
                if (it != m_vocab.end())
                    ids.push_back(it->second);
            }
            word.clear();
        }
    }
    return ids;
}

std::vector<std::pair<std::string, std::string>> CBPETokenizer::TopMerges(std::size_t n) const
{
    std::size_t take = std::min(n, m_merges.size());
    return {m_merges.begin(), m_merges.begin() + static_cast<std::ptrdiff_t>(take)};
}
