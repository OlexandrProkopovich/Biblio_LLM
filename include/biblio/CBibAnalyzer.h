#pragma once

#include "biblio/CArticleSource.h"

#include "core/tokenizer/CTokenizer.h"

#include "core/model/CEmbedding.h"
#include "core/model/CTransformerBlock.h"

#include "core/training/CTrainer.h"
#include "core/training/COptimizer.h"

#include <unordered_map>
#include <memory>

class CBibAnalyzer
{
public:
	CBibAnalyzer(CArticleSource* pSource, std::size_t dModel, std::size_t dK, std::size_t dFF);

	void Build(const std::vector<std::string>& queries);
	void Train(const std::vector<std::string>& queries, std::size_t epochs);

	// dataset mode: load query-article pairs from TSV (query\ttitle\tabstract or query\ttext)
	std::vector<std::pair<std::string, std::string>> LoadDataset(const std::string& path, std::size_t maxPairs = 0);
	void BuildFromPairs(const std::vector<std::pair<std::string, std::string>>& pairs);
	void TrainPairs(const std::vector<std::pair<std::string, std::string>>& pairs, std::size_t epochs, std::size_t batchSize = 1024, std::size_t numThreads = 0);

	std::vector<CArticle> Search(const std::string& query, std::size_t topN);
	std::vector<CArticle> SearchByEmbedding(const std::string& query, std::size_t topN);

	std::vector<CArticle> RankArticles(const std::string& query, const std::vector<CArticle>& articles, std::size_t topN);
	std::vector<CArticle> RankArticlesTFIDF(const std::string& query, const std::vector<CArticle>& articles, std::size_t topN);

	void SaveModel(const std::string& path);
	void LoadModel(const std::string& path);

	std::size_t ArticleCount() const;
	std::size_t VocabularySize() const;
	std::size_t EmbeddingDim() const;

private:
	std::unordered_map<std::string, float> ComputeTFIDF(const std::string& text);
	float CosineSimilarityMap(const std::unordered_map<std::string, float>& a, const std::unordered_map<std::string, float>& b);

private:
	CArticleSource* m_pSourceArticles;
	std::unique_ptr<CEmbedding> m_pEmbedding;
	std::unique_ptr<CTransformerBlock> m_pTransformer;

private:
	std::unordered_map<std::string, float> m_idf;
	std::vector<CArticle> m_articles;
	CTokenizer m_tokenizer;

	std::size_t m_dK;
	std::size_t m_dFF;
	std::size_t m_dModel;
};
