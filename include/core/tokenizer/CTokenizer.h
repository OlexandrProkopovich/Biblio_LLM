#pragma once
#include <unordered_map>
#include <string>
#include <vector>

class CTokenizer
{
public:
	CTokenizer() = default;

	void Build(const std::string& text);

	std::vector<std::size_t> Encode(const std::string& text) const;
	std::string Decode(const std::vector<std::size_t>& tokens) const;
	std::size_t VocabSize() const;

	void Save(std::ofstream& file) const;
	void Load(std::ifstream& file);

private:
	std::unordered_map<std::string, std::size_t> m_wordToIndex;
	std::unordered_map<std::size_t, std::string> m_indexToWord;
};