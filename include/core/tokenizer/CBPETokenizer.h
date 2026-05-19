#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

class CBPETokenizer
{
public:
    // Train BPE on raw text. numMerges controls vocab size beyond the base characters.
    void Build(const std::string& text, std::size_t numMerges = 1000);

    // Encode one word into subword strings (includes </w> end-of-word marker).
    std::vector<std::string> EncodeWord(const std::string& word) const;

    // Encode full text: splits on whitespace, encodes each word, returns token IDs.
    std::vector<std::size_t> Encode(const std::string& text) const;

    std::size_t VocabSize() const { return m_vocab.size(); }

    // Top-N most frequent merges (for display).
    std::vector<std::pair<std::string, std::string>> TopMerges(std::size_t n) const;

private:
    std::unordered_map<std::string, std::size_t> m_vocab;               // subword -> id
    std::vector<std::pair<std::string, std::string>>  m_merges;          // merge rules in order
};
