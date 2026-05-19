#pragma once
#include "core/model/CEmbedding.h"
#include "core/training/CLoss.h"
#include "core/training/COptimizer.h"
#include "biblio/CArticle.h"
#include <vector>
#include <string>
#include <thread>
#include <random>

class CTrainer
{
public:
    CTrainer(CEmbedding& embedding, COptimizer& optimizer,
             std::size_t batchSize = 32,
             std::size_t numThreads = std::thread::hardware_concurrency());

    void Train(const std::vector<std::vector<std::size_t>>& queries,
               const std::vector<std::vector<std::size_t>>& articles,
               std::size_t epochs);

private:
    CMatrix ComputeCosineGrad(const CMatrix& q, const CMatrix& other, float sim) const;

    // compute gradients for a range [from, to) of samples into localGrad
    void ComputeBatchGradients(
        const std::vector<std::vector<std::size_t>>& queries,
        const std::vector<std::vector<std::size_t>>& articles,
        const std::vector<CMatrix>& articleVecs,
        std::size_t from, std::size_t to,
        std::mt19937& rng,
        CMatrix& localGrad) const;

private:
    CEmbedding&  m_embedding;
    COptimizer&  m_optimizer;
    std::size_t  m_batchSize;
    std::size_t  m_numThreads;
};
