#include "biblio/CBibAnalyzer.h"
#include "core/math/CMatrix_ops.h"
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>

CBibAnalyzer::CBibAnalyzer(CArticleSource* pSource, std::size_t dModel, std::size_t dK, std::size_t dFF)
	:
	m_pSourceArticles(pSource),
	m_dModel(dModel),
	m_dK(dK),
	m_dFF(dFF)
{
}

void CBibAnalyzer::Train(const std::vector<std::string>& queries, std::size_t epochs)
{
	std::vector<std::vector<std::size_t>> tokenizedQueries;
	std::vector<std::vector<std::size_t>> tokenizedArticles;

	std::size_t articlesPerQuery = queries.empty() ? 0 : m_articles.size() / queries.size();

	if (articlesPerQuery == 0)
	{
		std::cerr << "Not enough articles to train (total=" << m_articles.size()
		          << ", queries=" << queries.size() << ")\n";
		return;
	}

	for (std::size_t q = 0; q < queries.size(); q++)
	{
		auto tokenizedQuery = m_tokenizer.Encode(queries[q]);
		if (tokenizedQuery.empty()) continue;

		for (std::size_t a = 0; a < articlesPerQuery; a++)
		{
			auto tokenizedArticle = m_tokenizer.Encode(
				m_articles[q * articlesPerQuery + a].title + " " +
				m_articles[q * articlesPerQuery + a].abstract);

			if (!tokenizedArticle.empty())
			{
				tokenizedQueries.push_back(tokenizedQuery);
				tokenizedArticles.push_back(tokenizedArticle);
			}
		}
	}

	std::cout << "Training pairs: " << tokenizedQueries.size() << "\n";

	COptimizer optimizer(0.001f);
	CTrainer trainer(*m_pEmbedding, optimizer);
	trainer.Train(tokenizedQueries, tokenizedArticles, epochs);
}

void CBibAnalyzer::Build(const std::vector<std::string>& queries)
{
	for (const auto& query : queries)
	{
		auto articles = m_pSourceArticles->GetArticles(query);
		m_articles.insert(m_articles.end(), articles.begin(), articles.end());
		std::cout << "Fetched: " << query << " (" << articles.size() << " articles)\n";
	}

	std::size_t totalDocs = m_articles.size();
	std::unordered_map<std::string, std::size_t> docCount;

	for (const auto& article : m_articles)
	{
		std::string text = article.title + " " + article.abstract;
		std::unordered_set<std::string> seenWords;
		std::string word;

		for (char c : text)
		{
			if (c != ' ')
				word += c;
			else
			{
				if (!word.empty() && seenWords.find(word) == seenWords.end())
				{
					docCount[word]++;
					seenWords.insert(word);
				}

				word.clear();
			}
		}

		if (!word.empty() && seenWords.find(word) == seenWords.end())
		{
			docCount[word]++;
			seenWords.insert(word);
		}
	}

	for (const auto& pair : docCount)
		m_idf[pair.first] = std::log(static_cast<float>(totalDocs) / pair.second);

	std::string allText;
	for (const auto& article : m_articles)
		allText += article.title + " " + article.abstract + " ";

	m_tokenizer.Build(allText);

	m_pEmbedding = std::make_unique<CEmbedding>(m_tokenizer.VocabSize(), m_dModel);
	m_pEmbedding->InitializeRandom();

	m_pTransformer = std::make_unique<CTransformerBlock>(m_dModel, m_dK, m_dFF);
}

std::vector<std::pair<std::string, std::string>> CBibAnalyzer::LoadDataset(const std::string& path, std::size_t maxPairs)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "Cannot open dataset: " << path << "\n";
		return {};
	}

	std::vector<std::pair<std::string, std::string>> pairs;
	if (maxPairs > 0)
		pairs.reserve(maxPairs);

	std::string line;
	std::size_t validRows = 0;
	std::mt19937 rng(42);

	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream ss(line);
		std::string query, col2, col3;

		if (!std::getline(ss, query, '\t')) continue;
		if (!std::getline(ss, col2, '\t'))  continue;

		std::string text = std::getline(ss, col3, '\t') ? col2 + " " + col3 : col2;
		if (query.empty() || text.empty())
			continue;

		validRows++;
		if (maxPairs == 0 || pairs.size() < maxPairs)
		{
			pairs.emplace_back(query, text);
		}
		else
		{
			std::uniform_int_distribution<std::size_t> dist(0, validRows - 1);
			std::size_t slot = dist(rng);
			if (slot < maxPairs)
				pairs[slot] = { query, text };
		}
	}

	std::cout << "Loaded " << pairs.size() << " sampled pairs from " << validRows
	          << " valid rows in " << path << "\n";
	return pairs;
}
void CBibAnalyzer::BuildFromPairs(const std::vector<std::pair<std::string, std::string>>& pairs)
{
	m_articles.clear();

	std::string allText;
	std::size_t totalDocs = pairs.size();
	std::unordered_map<std::string, std::size_t> docCount;

	for (const auto& [query, text] : pairs)
	{
		CArticle article;
		article.title    = text.substr(0, std::min(text.size(), std::size_t(120)));
		article.abstract = text;
		m_articles.push_back(article);

		std::unordered_set<std::string> seenWords;
		std::string word;
		for (char c : text + " " + query)
		{
			if (c != ' ') { word += c; }
			else
			{
				if (!word.empty() && !seenWords.count(word))
				{
					docCount[word]++;
					seenWords.insert(word);
				}
				word.clear();
			}
		}

		allText += query + " " + text + " ";
	}

	for (const auto& [w, cnt] : docCount)
		m_idf[w] = std::log(static_cast<float>(totalDocs) / cnt);

	m_tokenizer.Build(allText);

	m_pEmbedding = std::make_unique<CEmbedding>(m_tokenizer.VocabSize(), m_dModel);
	m_pEmbedding->InitializeRandom();

	m_pTransformer = std::make_unique<CTransformerBlock>(m_dModel, m_dK, m_dFF);

	std::cout << "Vocabulary size: " << m_tokenizer.VocabSize() << "\n";
	std::cout << "Articles loaded: " << m_articles.size() << "\n";
}

void CBibAnalyzer::TrainPairs(const std::vector<std::pair<std::string, std::string>>& pairs, std::size_t epochs, std::size_t batchSize, std::size_t numThreads)
{
	std::vector<std::vector<std::size_t>> tokenizedQueries;
	std::vector<std::vector<std::size_t>> tokenizedArticles;

	auto trainPairs = pairs;
	std::shuffle(trainPairs.begin(), trainPairs.end(), std::mt19937(1337));

	for (const auto& [query, text] : trainPairs)
	{
		auto qTokens = m_tokenizer.Encode(query);
		auto aTokens = m_tokenizer.Encode(text);
		if (!qTokens.empty() && !aTokens.empty())
		{
			tokenizedQueries.push_back(qTokens);
			tokenizedArticles.push_back(aTokens);
		}
	}

	std::cout << "Training pairs: " << tokenizedQueries.size() << "\n";

	COptimizer optimizer(0.001f);
	CTrainer trainer(*m_pEmbedding, optimizer, batchSize, numThreads == 0 ? std::thread::hardware_concurrency() : numThreads);
	trainer.Train(tokenizedQueries, tokenizedArticles, epochs);
}
std::vector<CArticle> CBibAnalyzer::Search(const std::string & query, std::size_t topN)
{
	auto queryVec = ComputeTFIDF(query);

	std::vector<std::pair<float, std::size_t>> scores;

	for (std::size_t i = 0; i < m_articles.size(); i++)
	{
		std::string text = m_articles[i].title + " " + m_articles[i].abstract;
		auto articleVec = ComputeTFIDF(text);
		float score = CosineSimilarityMap(queryVec, articleVec);
		scores.push_back({ score, i });
	}

	std::sort(scores.begin(), scores.end(),
		[](const auto& a, const auto& b) { return a.first > b.first; });

	std::vector<CArticle> result;
	for (std::size_t i = 0; i < std::min(topN, scores.size()); i++)
		result.push_back(m_articles[scores[i].second]);

	return result;
}

std::vector<CArticle> CBibAnalyzer::SearchByEmbedding(const std::string& query, std::size_t topN)
{
	auto queryTokens = m_tokenizer.Encode(query);
	CMatrix queryVec = m_pEmbedding->Encode(queryTokens);

	std::vector<std::pair<float, std::size_t>> scores;

	for (std::size_t i = 0; i < m_articles.size(); i++)
	{
		std::string text = m_articles[i].title + " " + m_articles[i].abstract;
		auto articleTokens = m_tokenizer.Encode(text);
		CMatrix articleVec = m_pEmbedding->Encode(articleTokens);

		float score = CosineSimilarity(queryVec, articleVec);
		scores.push_back({ score, i });
	}

	std::sort(scores.begin(), scores.end(),
		[](const auto& a, const auto& b) { return a.first > b.first; });

	std::vector<CArticle> result;
	for (std::size_t i = 0; i < std::min(topN, scores.size()); i++)
		result.push_back(m_articles[scores[i].second]);

	return result;
}

std::vector<CArticle> CBibAnalyzer::RankArticles(const std::string& query, const std::vector<CArticle>& articles, std::size_t topN)
{
	auto queryTokens = m_tokenizer.Encode(query);
	if (queryTokens.empty())
	{
		std::size_t n = std::min(topN, articles.size());
		return std::vector<CArticle>(articles.begin(), articles.begin() + n);
	}
	CMatrix queryVec = m_pEmbedding->Encode(queryTokens);

	std::vector<std::pair<float, std::size_t>> scores;

	for (std::size_t i = 0; i < articles.size(); i++)
	{
		std::string text = articles[i].title + " " + articles[i].abstract;
		auto articleTokens = m_tokenizer.Encode(text);
		CMatrix articleVec = m_pEmbedding->Encode(articleTokens);

		float score = CosineSimilarity(queryVec, articleVec);
		scores.push_back({ score, i });
	}

	std::sort(scores.begin(), scores.end(),
		[](const auto& a, const auto& b) { return a.first > b.first; });

	std::vector<CArticle> result;
	for (std::size_t i = 0; i < std::min(topN, scores.size()); i++)
		result.push_back(articles[scores[i].second]);

	return result;
}

std::vector<CArticle> CBibAnalyzer::RankArticlesTFIDF(const std::string& query, const std::vector<CArticle>& articles, std::size_t topN)
{
	if (articles.empty()) return {};

	// Build IDF from the current article set so ranking is domain-agnostic
	std::unordered_map<std::string, std::size_t> docFreq;
	std::size_t N = articles.size();

	auto tokenize = [](const std::string& text) -> std::vector<std::string>
	{
		std::vector<std::string> tokens;
		std::string word;
		for (unsigned char c : text)
		{
			if (std::isalpha(c))
				word += static_cast<char>(std::tolower(c));
			else if (!word.empty())
			{
				tokens.push_back(word);
				word.clear();
			}
		}
		if (!word.empty()) tokens.push_back(word);
		return tokens;
	};

	for (const auto& article : articles)
	{
		std::unordered_set<std::string> seen;
		for (const auto& w : tokenize(article.title + " " + article.abstract))
			if (seen.insert(w).second)
				docFreq[w]++;
	}

	auto computeLocalTFIDF = [&](const std::string& text) -> std::unordered_map<std::string, float>
	{
		std::unordered_map<std::string, std::size_t> wordCount;
		std::size_t total = 0;
		for (const auto& w : tokenize(text))
		{
			wordCount[w]++;
			total++;
		}
		std::unordered_map<std::string, float> result;
		for (const auto& [w, cnt] : wordCount)
		{
			float tf = static_cast<float>(cnt) / static_cast<float>(total);
			// +1 smoothing to avoid division by zero
			float idf = std::log(static_cast<float>(N + 1) / static_cast<float>(docFreq.count(w) ? docFreq.at(w) : 1));
			result[w] = tf * idf;
		}
		return result;
	};

	auto queryVec = computeLocalTFIDF(query);

	std::vector<std::pair<float, std::size_t>> scores;
	for (std::size_t i = 0; i < articles.size(); i++)
	{
		auto articleVec = computeLocalTFIDF(articles[i].title + " " + articles[i].abstract);
		float score = CosineSimilarityMap(queryVec, articleVec);
		scores.push_back({ score, i });
	}

	std::sort(scores.begin(), scores.end(),
		[](const auto& a, const auto& b) { return a.first > b.first; });

	std::vector<CArticle> result;
	for (std::size_t i = 0; i < std::min(topN, scores.size()); i++)
		result.push_back(articles[scores[i].second]);

	return result;
}

std::unordered_map<std::string, float> CBibAnalyzer::ComputeTFIDF(const std::string& text)
{
	std::unordered_map<std::string, std::size_t> wordCount;
	std::size_t totalWords = 0;

	std::string word;
	for (char c : text)
	{
		if (c != ' ')
			word += c;
		else
		{
			if (!word.empty())
			{
				wordCount[word]++;
				totalWords++;
			}
			word.clear();
		}
	}

	if (!word.empty())
	{
		wordCount[word]++;
		totalWords++;
	}

	std::unordered_map<std::string, float> result;
	for (const auto& pair : wordCount)
	{
		float tf = static_cast<float>(pair.second) / totalWords;
		float idf = m_idf.count(pair.first) ? m_idf.at(pair.first) : 0.0f;

		result[pair.first] = tf * idf;
	}

	return result;
}

float CBibAnalyzer::CosineSimilarityMap(const std::unordered_map<std::string, float>& a, const std::unordered_map<std::string, float>& b)
{
	float dot = 0.0f;
	float normA = 0.0f;
	float normB = 0.0f;

	for (const auto& pair : a)
	{
		if (b.count(pair.first))
			dot += pair.second * b.at(pair.first);

		normA += pair.second * pair.second;
	}

	for (const auto& pair : b)
		normB += pair.second * pair.second;

	normA = std::sqrt(normA);
	normB = std::sqrt(normB);

	if (normA == 0 || normB == 0) return 0.0f;

	return dot / (normA * normB);
}

void CBibAnalyzer::SaveModel(const std::string& path) 
{
	std::ofstream file(path, std::ios::binary);

	// статті
	std::size_t articleCount = m_articles.size();
	file.write(reinterpret_cast<const char*>(&articleCount), sizeof(articleCount));
	for (const auto& a : m_articles) {
		auto writeStr = [&](const std::string& s) {
			std::size_t len = s.size();
			file.write(reinterpret_cast<const char*>(&len), sizeof(len));
			file.write(s.data(), len);
		};
		writeStr(a.title);
		writeStr(a.abstract);
		writeStr(a.url);
		writeStr(a.published);
		std::size_t authCount = a.authors.size();
		file.write(reinterpret_cast<const char*>(&authCount), sizeof(authCount));
		for (const auto& auth : a.authors) writeStr(auth);
	}

	// IDF
	std::size_t idfSize = m_idf.size();
	file.write(reinterpret_cast<const char*>(&idfSize), sizeof(idfSize));
	for (const auto& pair : m_idf) {
		std::size_t len = pair.first.size();
		file.write(reinterpret_cast<const char*>(&len), sizeof(len));
		file.write(pair.first.data(), len);
		file.write(reinterpret_cast<const char*>(&pair.second), sizeof(float));
	}

	// токенайзер
	m_tokenizer.Save(file);

	// embedding
	m_pEmbedding->Save(file);
}

void CBibAnalyzer::LoadModel(const std::string& path) {
	std::ifstream file(path, std::ios::binary);

	// статті
	std::size_t articleCount;
	file.read(reinterpret_cast<char*>(&articleCount), sizeof(articleCount));
	m_articles.resize(articleCount);
	for (auto& a : m_articles) {
		auto readStr = [&](std::string& s) {
			std::size_t len;
			file.read(reinterpret_cast<char*>(&len), sizeof(len));
			s.resize(len);
			file.read(s.data(), len);
		};
		readStr(a.title);
		readStr(a.abstract);
		readStr(a.url);
		readStr(a.published);
		std::size_t authCount;
		file.read(reinterpret_cast<char*>(&authCount), sizeof(authCount));
		a.authors.resize(authCount);
		for (auto& auth : a.authors) readStr(auth);
	}

	// IDF
	std::size_t idfSize;
	file.read(reinterpret_cast<char*>(&idfSize), sizeof(idfSize));
	for (std::size_t i = 0; i < idfSize; i++) {
		std::size_t len;
		file.read(reinterpret_cast<char*>(&len), sizeof(len));
		std::string word(len, '\0');
		file.read(word.data(), len);
		float val;
		file.read(reinterpret_cast<char*>(&val), sizeof(float));
		m_idf[word] = val;
	}

	// токенайзер
	m_tokenizer.Load(file);

	// embedding
	m_pEmbedding = std::make_unique<CEmbedding>(1, 1);
	m_pEmbedding->Load(file);
	if (m_pEmbedding)
		 m_dModel = m_pEmbedding->GetWeights().GetCols();
}
std::size_t CBibAnalyzer::ArticleCount() const
{
	return m_articles.size();
}

std::size_t CBibAnalyzer::VocabularySize() const
{
	return m_tokenizer.VocabSize();
}

std::size_t CBibAnalyzer::EmbeddingDim() const
{
	return m_dModel;
}