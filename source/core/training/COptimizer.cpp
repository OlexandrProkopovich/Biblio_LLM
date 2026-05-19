#include "core/training/COptimizer.h"
#include <cmath>

COptimizer::COptimizer(float learningRate, float beta1, float beta2, float eps)
    : m_lr(learningRate), m_beta1(beta1), m_beta2(beta2), m_eps(eps)
{}

void COptimizer::Step(CMatrix& weights, const CMatrix& gradients)
{
    assert(weights.GetRows() == gradients.GetRows() && weights.GetCols() == gradients.GetCols());

    if (!m_initialized)
    {
        m_m.Reinitialize(weights.GetRows(), weights.GetCols()); // zeros
        m_v.Reinitialize(weights.GetRows(), weights.GetCols()); // zeros
        m_initialized = true;
    }

    m_t++;
    float bc1 = 1.0f - std::pow(m_beta1, static_cast<float>(m_t));
    float bc2 = 1.0f - std::pow(m_beta2, static_cast<float>(m_t));

    for (std::size_t i = 0; i < weights.GetRows(); i++)
    {
        for (std::size_t j = 0; j < weights.GetCols(); j++)
        {
            float g = gradients(i, j);
            m_m(i, j) = m_beta1 * m_m(i, j) + (1.0f - m_beta1) * g;
            m_v(i, j) = m_beta2 * m_v(i, j) + (1.0f - m_beta2) * g * g;

            float mHat = m_m(i, j) / bc1;
            float vHat = m_v(i, j) / bc2;

            weights(i, j) -= m_lr * mHat / (std::sqrt(vHat) + m_eps);
        }
    }
}

void COptimizer::Reset()
{
    m_t = 0;
    m_initialized = false;
}
