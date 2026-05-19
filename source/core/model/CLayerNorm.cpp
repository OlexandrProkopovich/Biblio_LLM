#include "core/model/CLayerNorm.h"
#include <cmath>

CLayerNorm::CLayerNorm(std::size_t dModel, float eps)
    : m_dModel(dModel), m_eps(eps)
{
    m_gamma.Reinitialize(1, dModel);
    m_beta.Reinitialize(1, dModel);
    for (std::size_t j = 0; j < dModel; j++)
        m_gamma(0, j) = 1.0f; // beta stays 0
}

CMatrix CLayerNorm::Forward(const CMatrix& X)
{
    std::size_t rows = X.GetRows();
    std::size_t cols = X.GetCols();
    CMatrix result(rows, cols);

    for (std::size_t i = 0; i < rows; i++)
    {
        float mean = 0.0f;
        for (std::size_t j = 0; j < cols; j++)
            mean += X(i, j);
        mean /= static_cast<float>(cols);

        float var = 0.0f;
        for (std::size_t j = 0; j < cols; j++)
        {
            float d = X(i, j) - mean;
            var += d * d;
        }
        var /= static_cast<float>(cols);
        float inv = 1.0f / std::sqrt(var + m_eps);

        for (std::size_t j = 0; j < cols; j++)
            result(i, j) = m_gamma(0, j) * (X(i, j) - mean) * inv + m_beta(0, j);
    }

    return result;
}
