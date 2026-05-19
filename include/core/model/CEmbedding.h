#pragma once

#include "core/math/CMatrix.h"
#include <string>

class CEmbedding
{
public:
	CEmbedding() = default;
	CEmbedding(std::size_t nVocabSize, std::size_t nDModel);

	void InitializeRandom();
	CMatrix GetEmbedding(std::size_t tokenID) const;

	CMatrix Encode(const std::vector<std::size_t>& tokens) const;
	CMatrix& GetWeights();

	void Save(std::ofstream& file);
	void Load(std::ifstream& file);

private:
	CMatrix m_weights;

	std::size_t m_nVocabSize;
	std::size_t m_nDModel;
};