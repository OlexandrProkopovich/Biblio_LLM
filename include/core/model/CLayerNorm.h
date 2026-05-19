#pragma once
#include "core/math/CMatrix.h"

class CLayerNorm
{
public:
    CLayerNorm() = default;
    CLayerNorm(std::size_t dModel, float eps = 1e-5f);

    CMatrix Forward(const CMatrix& X);

private:
    std::size_t m_dModel = 0;
    float m_eps = 1e-5f;
    CMatrix m_gamma; // scale (1 × dModel), initialized to 1
    CMatrix m_beta;  // shift (1 × dModel), initialized to 0
};
