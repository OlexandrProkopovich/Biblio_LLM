#include "core/model/CEmbedding.h"
#include <random>
#include <fstream>

CEmbedding::CEmbedding(std::size_t nVocabSize, std::size_t nDModel)
	:
	m_nVocabSize(nVocabSize),
	m_nDModel(nDModel)
{
	m_weights.Reinitialize(nVocabSize, nDModel);
}

void CEmbedding::InitializeRandom()
{
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

	for (std::size_t i = 0; i < m_nVocabSize; i++)
	{
		for (std::size_t j = 0; j < m_nDModel; j++)
		{
			m_weights(i, j) = dist(gen);
		}
	}
}

CMatrix CEmbedding::GetEmbedding(std::size_t tokenID) const
{
	if (tokenID >= m_nVocabSize) std::abort();

	CMatrix result(1, m_nDModel);

	for (std::size_t j = 0; j < m_nDModel; j++)
		result(0, j) = m_weights(tokenID, j);

	return result;
}

CMatrix CEmbedding::Encode(const std::vector<std::size_t>& tokens) const
{
	CMatrix result(1, m_nDModel);

	if (tokens.empty())
		return result;

	for (std::size_t token : tokens)
	{
		CMatrix emb = GetEmbedding(token);
		result += emb;
	}

	result /= static_cast<float>(tokens.size());
	return result;
}

CMatrix& CEmbedding::GetWeights()
{
	return m_weights;
}

void CEmbedding::Save(std::ofstream& file) 
{
	file.write(reinterpret_cast<const char*>(&m_nVocabSize), sizeof(m_nVocabSize));
	file.write(reinterpret_cast<const char*>(&m_nDModel), sizeof(m_nDModel));

	for (std::size_t i = 0; i < m_nVocabSize; i++)
	{
		for (std::size_t j = 0; j < m_nDModel; j++)
		{
			float val = m_weights(i, j);
			file.write(reinterpret_cast<const char*>(&val), sizeof(float));
		}
	}
}

void CEmbedding::Load(std::ifstream& file) 
{
	file.read(reinterpret_cast<char*>(&m_nVocabSize), sizeof(m_nVocabSize));
	file.read(reinterpret_cast<char*>(&m_nDModel), sizeof(m_nDModel));

	m_weights.Reinitialize(m_nVocabSize, m_nDModel);

	for (std::size_t i = 0; i < m_nVocabSize; i++) 
	{
		for (std::size_t j = 0; j < m_nDModel; j++)
		{
			float val;
			file.read(reinterpret_cast<char*>(&val), sizeof(float));
			m_weights(i, j) = val;
		}
	}
}