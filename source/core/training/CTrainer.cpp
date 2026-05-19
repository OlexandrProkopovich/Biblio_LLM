#include "core/training/CTrainer.h"
#include "core/math/CMatrix_ops.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>

CTrainer::CTrainer(CEmbedding& embedding, COptimizer& optimizer,
                   std::size_t batchSize, std::size_t numThreads)
    : m_embedding(embedding),
      m_optimizer(optimizer),
      m_batchSize(batchSize),
      m_numThreads(std::max(std::size_t(1), numThreads))
{}

void CTrainer::ComputeBatchGradients(
    const std::vector<std::vector<std::size_t>>& queries,
    const std::vector<std::vector<std::size_t>>& articles,
    const std::vector<CMatrix>& articleVecs,
    std::size_t from, std::size_t to,
    std::mt19937& rng,
    CMatrix& localGrad) const
{
    const int NUM_NEG_CANDIDATES = 7;
    std::uniform_int_distribution<std::size_t> dist(0, articles.size() - 1);

    for (std::size_t i = from; i < to; i++)
    {
        CMatrix qVec   = m_embedding.Encode(queries[i]);
        CMatrix posVec = articleVecs[i];

        // hard negative: sample candidates, pick highest cosine sim with query
        float hardestSim = -2.0f;
        std::size_t negIdx = (i + 1) % articles.size();

        for (int k = 0; k < NUM_NEG_CANDIDATES; k++)
        {
            std::size_t candidate = dist(rng);
            if (candidate == i) continue;
            float sim = CosineSimilarity(qVec, articleVecs[candidate]);
            if (sim > hardestSim)
            {
                hardestSim = sim;
                negIdx = candidate;
            }
        }
        CMatrix negVec = articleVecs[negIdx];

        float simPos = CosineSimilarity(qVec, posVec);
        float simNeg = CosineSimilarity(qVec, negVec);
        float loss   = CLoss::ContrastiveLoss(simPos, simNeg);

        if (loss <= 0.0f) continue;

        std::size_t dModel = m_embedding.GetWeights().GetCols();

        // query: pull toward positive and push away from hard negative
        CMatrix gradQPos = ComputeCosineGrad(qVec, posVec, simPos);
        CMatrix gradQNeg = ComputeCosineGrad(qVec, negVec, simNeg);
        for (std::size_t token : queries[i])
            for (std::size_t j = 0; j < dModel; j++)
                localGrad(token, j) += (gradQNeg(0, j) - gradQPos(0, j)) / static_cast<float>(queries[i].size());

        // positive article: pull toward query
        CMatrix gradPos = ComputeCosineGrad(posVec, qVec, simPos);
        for (std::size_t token : articles[i])
            for (std::size_t j = 0; j < dModel; j++)
                localGrad(token, j) -= gradPos(0, j) / static_cast<float>(articles[i].size());

        // negative article: push away from query
        CMatrix gradNeg = ComputeCosineGrad(negVec, qVec, simNeg);
        for (std::size_t token : articles[negIdx])
            for (std::size_t j = 0; j < dModel; j++)
                localGrad(token, j) += gradNeg(0, j) / static_cast<float>(articles[negIdx].size());
    }
}

void CTrainer::Train(const std::vector<std::vector<std::size_t>>& queries,
                     const std::vector<std::vector<std::size_t>>& articles,
                     std::size_t epochs)
{
    const std::size_t N       = queries.size();
    const std::size_t vocab   = m_embedding.GetWeights().GetRows();
    const std::size_t dModel  = m_embedding.GetWeights().GetCols();

    std::cout << "Threads: " << m_numThreads
              << "  Batch size: " << m_batchSize << "\n";

    for (std::size_t epoch = 0; epoch < epochs; epoch++)
    {
        // --- parallel precompute all article embeddings ---
        std::vector<CMatrix> articleVecs(N);
        {
            std::size_t chunk = (N + m_numThreads - 1) / m_numThreads;
            std::vector<std::thread> threads;

            for (std::size_t t = 0; t < m_numThreads; t++)
            {
                std::size_t from = t * chunk;
                std::size_t to   = std::min(from + chunk, N);
                if (from >= N) break;

                threads.emplace_back([&, from, to]() {
                    for (std::size_t i = from; i < to; i++)
                        articleVecs[i] = m_embedding.Encode(articles[i]);
                });
            }
            for (auto& th : threads) th.join();
        }

        // --- mini-batch training loop ---
        float totalLoss = 0.0f;
        std::mt19937 rng(epoch); // deterministic per epoch

        for (std::size_t batchStart = 0; batchStart < N; batchStart += m_batchSize)
        {
            std::size_t batchEnd = std::min(batchStart + m_batchSize, N);
            std::size_t batchLen = batchEnd - batchStart;

            // split batch across threads, each accumulates into its own gradient matrix
            std::size_t chunk = (batchLen + m_numThreads - 1) / m_numThreads;
            std::vector<CMatrix> threadGrads(m_numThreads, CMatrix(vocab, dModel));
            std::vector<std::thread> threads;

            for (std::size_t t = 0; t < m_numThreads; t++)
            {
                std::size_t from = batchStart + t * chunk;
                std::size_t to   = std::min(from + chunk, batchEnd);
                if (from >= batchEnd) break;

                std::mt19937 threadRng(epoch * 1000 + t);
                threads.emplace_back([&, t, from, to, threadRng]() mutable {
                    ComputeBatchGradients(queries, articles, articleVecs,
                                         from, to, threadRng, threadGrads[t]);
                });
            }
            for (auto& th : threads) th.join();

            // merge gradients from all threads
            CMatrix totalGrad(vocab, dModel);
            for (auto& tg : threadGrads)
                totalGrad += tg;

            m_optimizer.Step(m_embedding.GetWeights(), totalGrad);

            // accumulate loss for reporting
            for (std::size_t i = batchStart; i < batchEnd; i++)
            {
                float simPos = CosineSimilarity(m_embedding.Encode(queries[i]), articleVecs[i]);
                float simNeg = CosineSimilarity(m_embedding.Encode(queries[i]),
                                                articleVecs[(i + 1) % N]);
                totalLoss += CLoss::ContrastiveLoss(simPos, simNeg);
            }
        }

        std::cout << "Epoch " << epoch << "  loss: " << totalLoss << "\n";
    }
}

CMatrix CTrainer::ComputeCosineGrad(const CMatrix& q, const CMatrix& other, float sim) const
{
    float normQ = 0.0f, normOther = 0.0f;
    for (std::size_t j = 0; j < q.GetCols(); j++)
    {
        normQ     += q(0, j)     * q(0, j);
        normOther += other(0, j) * other(0, j);
    }
    normQ     = std::sqrt(normQ);
    normOther = std::sqrt(normOther);

    float denom = normQ * normOther;
    if (denom < 1e-8f) return CMatrix(1, q.GetCols());

    CMatrix grad(1, q.GetCols());
    for (std::size_t j = 0; j < q.GetCols(); j++)
        grad(0, j) = (other(0, j) - sim * q(0, j)) / denom;

    return grad;
}
