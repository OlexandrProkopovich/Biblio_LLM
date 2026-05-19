#pragma once

#include "core/math/CMatrix.h"
#include "core/model/CAttention.h"
#include "core/model/CFFLayer.h"
#include "core/model/CLayerNorm.h"

class CTransformerBlock
{
public:
    CTransformerBlock() = default;
    CTransformerBlock(std::size_t dModel, std::size_t dK, std::size_t dFF);

    CMatrix Forward(const CMatrix& X);

private:
    CAttention  m_attention;
    CFFLayer    m_ffLayer;
    CLayerNorm  m_norm1;
    CLayerNorm  m_norm2;
};
