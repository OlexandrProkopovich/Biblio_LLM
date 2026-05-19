#include "core/model/CTransformerBlock.h"
#include "core/math/CMatrix_ops.h"

CTransformerBlock::CTransformerBlock(std::size_t dModel, std::size_t dK, std::size_t dFF)
    : m_attention(dModel, dK),
      m_ffLayer(dModel, dFF),
      m_norm1(dModel),
      m_norm2(dModel)
{}

// Post-LN: residual first, then normalize (original "Attention is All You Need")
CMatrix CTransformerBlock::Forward(const CMatrix& X)
{
    CMatrix attnOut  = m_attention.Forward(X);
    CMatrix residual1 = m_norm1.Forward(X + attnOut);

    CMatrix ffOut    = m_ffLayer.Forward(residual1);
    CMatrix residual2 = m_norm2.Forward(residual1 + ffOut);

    return residual2;
}
