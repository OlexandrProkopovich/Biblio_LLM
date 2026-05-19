#include "core/math/CMatrix_ops.h"
#include <cmath>

CMatrix operator+(const CMatrix& a, const CMatrix& b)
{
	std::size_t aRows = a.GetRows();
	std::size_t aCols = a.GetCols();

	std::size_t bRows = b.GetRows();
	std::size_t bCols = b.GetCols();

	assert(aRows == bRows && aCols == bCols);
	
	CMatrix result(aRows, aCols);
	for (std::size_t i = 0; i < aRows; i++)
	{
		for (std::size_t j = 0; j < aCols; j++)
		{
			result(i, j) = a(i, j) + b(i, j);
		}
	}

	return result;
}

CMatrix operator-(const CMatrix& a, const CMatrix& b)
{
	std::size_t aRows = a.GetRows();
	std::size_t aCols = a.GetCols();

	std::size_t bRows = b.GetRows();
	std::size_t bCols = b.GetCols();

	assert(aRows == bRows && aCols == bCols);

	CMatrix result(aRows, aCols);
	for (std::size_t i = 0; i < aRows; i++)
	{
		for (std::size_t j = 0; j < aCols; j++)
		{
			result(i, j) = a(i, j) - b(i, j);
		}
	}

	return result;
}

CMatrix operator*(const CMatrix& a, const CMatrix& b)
{
	if (a.GetCols() != b.GetRows()) std::abort();

	CMatrix result(a.GetRows(), b.GetCols());

	for (std::size_t i = 0; i < a.GetRows(); i++)
	{
		for (std::size_t j = 0; j < b.GetCols(); j++)
		{
			for (std::size_t k = 0; k < a.GetCols(); k++)
			{
				result(i, j) += a(i, k) * b(k, j);
			}
		}
	}

	return result;
}

float CosineSimilarity(const CMatrix& a, const CMatrix& b)
{
	assert(a.GetCols() == b.GetCols());

	float dot = 0.0f;
	float normA = 0.0f;
	float normB = 0.0f;

	for (std::size_t j = 0; j < a.GetCols(); j++)
	{
		dot += a(0, j) * b(0, j);
		normA += a(0, j) * a(0, j);
		normB += b(0, j) * b(0, j);
	}

	normA = std::sqrt(normA);
	normB = std::sqrt(normB);

	if (normA == 0.0f || normB == 0.0f) 
		return 0.0f;

	return dot / (normA * normB);
}