#include "core/math/CActivations.h"
#include <cmath>
#include <algorithm>

CMatrix Softmax(const CMatrix& Vector)
{
	if (Vector.GetCols() != 1) std::abort();

	CMatrix result(Vector.GetRows(), 1);
	float nMaxValue = Vector.GetMax();

	float sum = 0.0f;
	for (std::size_t i = 0; i < Vector.GetRows(); i++)
	{
		sum += std::exp(Vector(i, 0) - nMaxValue);
	}

	for (std::size_t i = 0; i < Vector.GetRows(); i++)
	{
		result(i, 0) = std::exp(Vector(i, 0) - nMaxValue) / sum;
	}

	return result;
}

CMatrix ReLu(const CMatrix& Vector)
{
	CMatrix result(Vector.GetRows(), Vector.GetCols());

	for (std::size_t i = 0; i < Vector.GetRows(); i++)
	{
		for (std::size_t j = 0; j < Vector.GetCols(); j++)
		{
			result(i, j) = std::max(0.f, Vector(i, j));
		}
	}

	return result;
}