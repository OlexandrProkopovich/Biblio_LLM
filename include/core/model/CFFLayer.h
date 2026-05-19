#pragma once

#include "core/math/CMatrix.h"

class CFFLayer
{
public:
	CFFLayer() = default;
	CFFLayer(std::size_t dModel, std::size_t dFF);

	CMatrix Forward(const CMatrix& X);

private:
	CMatrix m_W1;
	CMatrix m_W2;

	std::size_t m_dModel;
	std::size_t m_dFF;
};