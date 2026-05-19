#pragma once

#include "core/math/CMatrix.h"

class CAttention
{
public:
	CAttention() = default;
	CAttention(std::size_t dModel, std::size_t dK);

	CMatrix Forward(const CMatrix& X);

private:
	CMatrix m_Wq;
	CMatrix m_Wk;
	CMatrix m_Wv;

	std::size_t m_dK;
};