#include "core/model/CFFLayer.h"
#include "core/math/CMatrix_ops.h"
#include "core/math/CActivations.h"

CFFLayer::CFFLayer(std::size_t dModel, std::size_t dFF)
	:
	m_dModel(dModel),
	m_dFF(dFF)
{
	m_W1.Reinitialize(dModel, dFF);
	m_W1.InitializeRandom();

	m_W2.Reinitialize(dFF, dModel);
	m_W2.InitializeRandom();
}

CMatrix CFFLayer::Forward(const CMatrix& X)
{
	CMatrix hidden = X * m_W1;
	hidden = ReLu(hidden);

	return hidden * m_W2;
}