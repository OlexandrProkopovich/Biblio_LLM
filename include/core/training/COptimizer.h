#pragma once
#include "core/math/CMatrix.h"

class COptimizer
{
public:
    COptimizer(float learningRate = 0.001f, float beta1 = 0.9f, float beta2 = 0.999f, float eps = 1e-8f);

    void Step(CMatrix& weights, const CMatrix& gradients);
    void Reset();

private:
    float m_lr;
    float m_beta1;
    float m_beta2;
    float m_eps;

    std::size_t m_t = 0;
    CMatrix m_m; // first moment
    CMatrix m_v; // second moment
    bool m_initialized = false;
};
