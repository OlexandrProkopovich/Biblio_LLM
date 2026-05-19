#include "core/model/CAttention.h"
#include "core/math/CMatrix_ops.h"
#include "core/math/CActivations.h"
#include <cmath>

CAttention::CAttention(std::size_t dModel, std::size_t dK)
	:
	m_dK(dK)
{
	m_Wq.Reinitialize(dModel, dK);
	m_Wk.Reinitialize(dModel, dK);
	m_Wv.Reinitialize(dModel, dK);

	m_Wq.InitializeRandom();
	m_Wk.InitializeRandom();
	m_Wv.InitializeRandom();
}

CMatrix CAttention::Forward(const CMatrix& X)
{
	CMatrix Q = X * m_Wq;
	CMatrix K = X * m_Wk;
	CMatrix V = X * m_Wv;

	CMatrix scores = Q * K.Transpose();
	float scale = std::sqrt(static_cast<float>(m_dK));
	scores /= scale;

	for (std::size_t i = 0; i < scores.GetRows(); i++)
	{
		CMatrix row(scores.GetCols(), 1);
		for (std::size_t j = 0; j < scores.GetCols(); j++)
			row(j, 0) = scores(i, j);


		CMatrix softRow = Softmax(row);
		for (std::size_t j = 0; j < scores.GetCols(); j++)
			scores(i, j) = softRow(j, 0);
	}

	return scores * V;
}