// Diploma benchmark suite for the bibliographic reranking system.
// Benchmarks are written to ./benchmarks.
//
// Usage:
//   Benchmark.exe [model.bin] [dataset.tsv] [samples_per_topic]

#include "biblio/CBibAnalyzer.h"
#include "biblio/CArticle.h"
#include "core/tokenizer/CTokenizer.h"
#include "core/tokenizer/CBPETokenizer.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
static const std::filesystem::path kBenchmarkDir = "benchmarks";

static const std::vector<std::string> kTopics = {
    "quantum computing", "computer vision", "machine learning",
    "natural language processing", "combinatorics", "probability theory",
    "artificial intelligence", "number theory", "optimization control",
    "numerical analysis", "robotics", "cryptography security",
    "signal processing", "statistical methodology", "computational physics"
};

struct QueryCase
{
    std::string query;
    std::vector<std::string> relevantTopics;
    std::vector<std::string> hardNegativeTopics;
};

static const std::vector<QueryCase> kQueryCases = {
    {"neural model for image classification", {"machine learning", "computer vision", "artificial intelligence"}, {"signal processing", "statistical methodology", "optimization control"}},
    {"transformer language model retrieval", {"natural language processing", "machine learning", "information retrieval"}, {"artificial intelligence", "statistical methodology", "computer vision"}},
    {"quantum circuit optimization", {"quantum computing", "optimization control"}, {"computational physics", "numerical analysis", "probability theory"}},
    {"robot control reinforcement learning", {"robotics", "machine learning", "optimization control"}, {"artificial intelligence", "control systems", "signal processing"}},
    {"cryptographic protocol security", {"cryptography security"}, {"quantum computing", "computer networks", "number theory"}},
    {"graph combinatorics theorem", {"combinatorics", "number theory"}, {"probability theory", "optimization control", "machine learning"}},
    {"statistical inference probability model", {"statistical methodology", "probability theory", "machine learning"}, {"numerical analysis", "optimization control", "artificial intelligence"}},
    {"numerical simulation computational physics", {"computational physics", "numerical analysis"}, {"quantum computing", "optimization control", "signal processing"}}
};

struct TopicSamples
{
    std::unordered_map<std::string, std::vector<CArticle>> byTopic;
    std::unordered_map<std::string, std::size_t> seenByTopic;
    std::size_t validRows = 0;
};

struct RankingMetrics
{
    double ndcg10 = 0.0;
    double precision10 = 0.0;
    double mrr10 = 0.0;
};

static double msElapsed(Clock::time_point t0)
{
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t0).count()) / 1000.0;
}

static double usElapsed(Clock::time_point t0)
{
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t0).count());
}

static std::string fmt(double value, int precision = 4)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

static void writeCSV(const std::string& filename,
                     const std::vector<std::string>& headers,
                     const std::vector<std::vector<std::string>>& rows)
{
    std::filesystem::create_directories(kBenchmarkDir);
    std::filesystem::path outPath = kBenchmarkDir / filename;
    std::ofstream f(outPath);

    for (std::size_t i = 0; i < headers.size(); i++)
    {
        f << headers[i];
        if (i + 1 < headers.size()) f << ',';
    }
    f << '\n';

    for (const auto& row : rows)
    {
        for (std::size_t i = 0; i < row.size(); i++)
        {
            const std::string& cell = row[i];
            bool quote = cell.find(',') != std::string::npos || cell.find('"') != std::string::npos;
            if (quote)
            {
                f << '"';
                for (char c : cell)
                    f << (c == '"' ? "\"\"" : std::string(1, c));
                f << '"';
            }
            else
            {
                f << cell;
            }
            if (i + 1 < row.size()) f << ',';
        }
        f << '\n';
    }

    std::cout << "  wrote " << outPath.string() << "\n";
}

static void printHeader(const std::string& title)
{
    std::cout << "\n" << std::string(76, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(76, '=') << "\n";
}

static CArticle makeArticle(const std::string& label, const std::string& title,
                            const std::string& abstract, std::size_t id)
{
    CArticle a;
    a.title = title;
    a.abstract = abstract;
    a.url = label + "#" + std::to_string(id);
    return a;
}

static TopicSamples loadTopicSamples(const std::string& datasetPath, std::size_t perTopic)
{
    printHeader("Loading evaluation samples");

    std::unordered_set<std::string> topicSet(kTopics.begin(), kTopics.end());
    TopicSamples samples;
    for (const auto& t : kTopics)
    {
        samples.byTopic[t] = {};
        samples.seenByTopic[t] = 0;
    }

    std::ifstream file(datasetPath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open dataset: " + datasetPath);

    std::mt19937 rng(2026);
    std::string line;
    std::size_t rowId = 0;

    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string query, title, abstract;
        if (!std::getline(ss, query, '\t')) continue;
        if (!std::getline(ss, title, '\t')) continue;
        if (!std::getline(ss, abstract, '\t')) continue;
        if (!topicSet.count(query) || title.empty() || abstract.empty()) continue;

        samples.validRows++;
        auto& seen = samples.seenByTopic[query];
        seen++;

        CArticle article = makeArticle(query, title, abstract, rowId++);
        auto& bucket = samples.byTopic[query];
        if (bucket.size() < perTopic)
        {
            bucket.push_back(std::move(article));
        }
        else
        {
            std::uniform_int_distribution<std::size_t> dist(0, seen - 1);
            std::size_t slot = dist(rng);
            if (slot < perTopic)
                bucket[slot] = std::move(article);
        }
    }

    std::cout << "Selected-topic rows: " << samples.validRows << "\n";
    return samples;
}

static std::vector<CArticle> sampleFromBucket(const std::vector<CArticle>& bucket,
                                               std::size_t n,
                                               std::mt19937& rng)
{
    std::vector<CArticle> result;
    if (bucket.empty() || n == 0) return result;
    std::vector<std::size_t> idx(bucket.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);
    n = std::min(n, bucket.size());
    result.reserve(n);
    for (std::size_t i = 0; i < n; i++) result.push_back(bucket[idx[i]]);
    return result;
}

static bool hasTopic(const std::vector<std::string>& topics, const std::string& topic)
{
    return std::find(topics.begin(), topics.end(), topic) != topics.end();
}

static std::vector<CArticle> buildHardCandidateSet(const TopicSamples& samples,
                                                    const QueryCase& qc,
                                                    std::unordered_map<std::string, int>& relevance,
                                                    std::mt19937& rng)
{
    relevance.clear();
    std::vector<CArticle> candidates;

    for (const auto& t : qc.relevantTopics)
    {
        if (!samples.byTopic.count(t)) continue;
        auto chunk = sampleFromBucket(samples.byTopic.at(t), 4, rng);
        for (const auto& a : chunk)
        {
            candidates.push_back(a);
            relevance[a.url] = 1;
        }
    }

    for (const auto& t : qc.hardNegativeTopics)
    {
        if (!samples.byTopic.count(t)) continue;
        auto chunk = sampleFromBucket(samples.byTopic.at(t), 7, rng);
        for (const auto& a : chunk)
        {
            if (!relevance.count(a.url))
            {
                candidates.push_back(a);
                relevance[a.url] = 0;
            }
        }
    }

    std::vector<std::string> otherTopics = kTopics;
    std::shuffle(otherTopics.begin(), otherTopics.end(), rng);
    for (const auto& t : otherTopics)
    {
        if (candidates.size() >= 50) break;
        if (hasTopic(qc.relevantTopics, t) || hasTopic(qc.hardNegativeTopics, t)) continue;
        auto chunk = sampleFromBucket(samples.byTopic.at(t), 3, rng);
        for (const auto& a : chunk)
        {
            if (candidates.size() >= 50) break;
            if (!relevance.count(a.url))
            {
                candidates.push_back(a);
                relevance[a.url] = 0;
            }
        }
    }

    std::shuffle(candidates.begin(), candidates.end(), rng);
    return candidates;
}

static double dcg(const std::vector<int>& rels, std::size_t k)
{
    double score = 0.0;
    for (std::size_t i = 0; i < std::min(k, rels.size()); i++)
        score += static_cast<double>(rels[i]) / std::log2(static_cast<double>(i + 2));
    return score;
}

static RankingMetrics computeMetrics(const std::vector<CArticle>& ranked,
                                      const std::unordered_map<std::string, int>& relevance)
{
    constexpr std::size_t K = 10;
    std::vector<int> rels;
    rels.reserve(ranked.size());
    for (const auto& article : ranked)
    {
        auto it = relevance.find(article.url);
        rels.push_back(it == relevance.end() ? 0 : it->second);
    }

    std::vector<int> ideal = rels;
    std::sort(ideal.begin(), ideal.end(), std::greater<int>());

    RankingMetrics m;
    double idcg = dcg(ideal, K);
    m.ndcg10 = idcg == 0.0 ? 0.0 : dcg(rels, K) / idcg;

    int relevantTop10 = 0;
    for (std::size_t i = 0; i < std::min(K, rels.size()); i++)
        if (rels[i] > 0) relevantTop10++;
    m.precision10 = static_cast<double>(relevantTop10) / static_cast<double>(K);

    for (std::size_t i = 0; i < std::min(K, rels.size()); i++)
    {
        if (rels[i] > 0)
        {
            m.mrr10 = 1.0 / static_cast<double>(i + 1);
            break;
        }
    }
    return m;
}

static void bench1_ranking_quality(CBibAnalyzer& analyzer, const TopicSamples& samples)
{
    printHeader("Benchmark 1: Original vs TF-IDF vs Neural ranking quality on hard candidates");
    std::mt19937 rng(42);
    std::vector<std::vector<std::string>> rows;
    double oNdcg = 0, nNdcg = 0, tNdcg = 0, oP = 0, nP = 0, tP = 0;

    for (const auto& qc : kQueryCases)
    {
        std::unordered_map<std::string, int> relevance;
        auto candidates = buildHardCandidateSet(samples, qc, relevance, rng);
        auto rankedNeural = analyzer.RankArticles(qc.query, candidates, candidates.size());
        auto rankedTFIDF = analyzer.RankArticlesTFIDF(qc.query, candidates, candidates.size());
        RankingMetrics om = computeMetrics(candidates, relevance);
        RankingMetrics nm = computeMetrics(rankedNeural, relevance);
        RankingMetrics tm = computeMetrics(rankedTFIDF, relevance);

        oNdcg += om.ndcg10; nNdcg += nm.ndcg10; tNdcg += tm.ndcg10;
        oP += om.precision10; nP += nm.precision10; tP += tm.precision10;
        rows.push_back({
            qc.query,
            fmt(om.ndcg10), fmt(tm.ndcg10), fmt(nm.ndcg10),
            fmt(om.precision10), fmt(tm.precision10), fmt(nm.precision10),
            fmt(om.mrr10), fmt(tm.mrr10), fmt(nm.mrr10)
        });
    }

    double denom = static_cast<double>(kQueryCases.size());
    rows.push_back({
        "AVERAGE",
        fmt(oNdcg / denom), fmt(tNdcg / denom), fmt(nNdcg / denom),
        fmt(oP / denom), fmt(tP / denom), fmt(nP / denom),
        "", "", ""
    });

    writeCSV("bench1_neural_vs_tfidf_quality.csv",
        {
            "query",
            "original_ndcg10", "tfidf_ndcg10", "neural_ndcg10",
            "original_p10", "tfidf_p10", "neural_p10",
            "original_mrr10", "tfidf_mrr10", "neural_mrr10"
        },
        rows);
}

static std::vector<CArticle> mixedCandidates(const TopicSamples& samples, std::size_t n, std::mt19937& rng)
{
    std::vector<CArticle> result;
    result.reserve(n);
    std::size_t cursor = 0;
    while (result.size() < n)
    {
        const auto& topic = kTopics[cursor % kTopics.size()];
        const auto& bucket = samples.byTopic.at(topic);
        if (!bucket.empty())
        {
            std::uniform_int_distribution<std::size_t> dist(0, bucket.size() - 1);
            result.push_back(bucket[dist(rng)]);
        }
        cursor++;
    }
    std::shuffle(result.begin(), result.end(), rng);
    return result;
}

static void bench2_reranking_latency(CBibAnalyzer& analyzer, const TopicSamples& samples)
{
    printHeader("Benchmark 2: Neural vs TF-IDF reranking latency");
    std::vector<std::size_t> sizes = {50, 100, 150, 200, 300, 500, 1000};
    std::mt19937 rng(99);
    constexpr int REPS = 5;
    std::vector<std::vector<std::string>> rows;

    for (std::size_t n : sizes)
    {
        auto candidates = mixedCandidates(samples, n, rng);
        std::string query = "machine learning neural model";
        double neuralMs = 0.0, tfidfMs = 0.0;
        for (int r = 0; r < REPS; r++)
        {
            auto t0 = Clock::now();
            analyzer.RankArticles(query, candidates, candidates.size());
            neuralMs += msElapsed(t0);
            t0 = Clock::now();
            analyzer.RankArticlesTFIDF(query, candidates, candidates.size());
            tfidfMs += msElapsed(t0);
        }
        neuralMs /= REPS;
        tfidfMs /= REPS;
        rows.push_back({std::to_string(n), fmt(neuralMs, 3), fmt(tfidfMs, 3), fmt(neuralMs / tfidfMs, 3)});
    }

    writeCSV("bench2_neural_vs_tfidf_latency.csv",
        {"n_articles", "neural_ms", "tfidf_ms", "neural_to_tfidf_ratio"}, rows);
}

static std::string tokenizerCorpus()
{
    return "machine learning neural network transformer attention embedding retrieval ranking "
           "quantum computing entanglement circuit qubit optimization numerical analysis "
           "computer vision image classification segmentation robotics cryptography security "
           "probability statistics signal processing combinatorics number theory computational physics ";
}

static void bench3_word_vs_bpe_speed()
{
    printHeader("Benchmark 3: Word vs BPE tokenization speed");
    std::string corpus = tokenizerCorpus();
    CTokenizer wordTok;
    wordTok.Build(corpus);
    CBPETokenizer bpeTok;
    bpeTok.Build(corpus, 500);

    std::string text;
    while (text.size() < 4000) text += corpus;
    std::vector<std::size_t> lengths = {50, 100, 200, 500, 1000, 2000, 4000};
    constexpr int REPS = 5000;
    std::vector<std::vector<std::string>> rows;

    for (std::size_t len : lengths)
    {
        std::string sample = text.substr(0, len);
        volatile std::size_t sink = 0;
        auto t0 = Clock::now();
        for (int r = 0; r < REPS; r++) sink += wordTok.Encode(sample).size();
        double wordUs = usElapsed(t0) / REPS;
        t0 = Clock::now();
        for (int r = 0; r < REPS; r++) sink += bpeTok.Encode(sample).size();
        double bpeUs = usElapsed(t0) / REPS;
        rows.push_back({std::to_string(len), fmt(wordUs, 3), fmt(bpeUs, 3), fmt(bpeUs / wordUs, 2)});
    }

    writeCSV("bench3_word_vs_bpe_tokenization.csv", {"chars", "word_us", "bpe_us", "bpe_to_word_ratio"}, rows);
}

static std::vector<std::string> extractAlphaWords(const std::string& text, std::size_t maxWords)
{
    std::vector<std::string> words;
    words.reserve(std::min<std::size_t>(maxWords, 4096));

    std::string current;
    for (unsigned char ch : text)
    {
        if (std::isalpha(ch))
        {
            current.push_back(static_cast<char>(std::tolower(ch)));
        }
        else if (!current.empty())
        {
            if (current.size() >= 3)
                words.push_back(current);
            current.clear();
            if (words.size() >= maxWords)
                return words;
        }
    }

    if (!current.empty() && current.size() >= 3 && words.size() < maxWords)
        words.push_back(current);

    return words;
}

static void appendTopicWords(const TopicSamples& samples,
                             const std::vector<std::string>& topics,
                             std::size_t articleOffset,
                             std::size_t maxArticlesPerTopic,
                             std::size_t maxWords,
                             std::vector<std::string>& out)
{
    for (const auto& topic : topics)
    {
        auto it = samples.byTopic.find(topic);
        if (it == samples.byTopic.end())
            continue;

        const auto& bucket = it->second;
        std::size_t end = std::min(bucket.size(), articleOffset + maxArticlesPerTopic);
        for (std::size_t i = articleOffset; i < end && out.size() < maxWords; i++)
        {
            const auto text = bucket[i].title + " " + bucket[i].abstract;
            auto words = extractAlphaWords(text, maxWords - out.size());
            out.insert(out.end(), words.begin(), words.end());
        }
    }
}

static void bench4_oov_coverage(const TopicSamples& samples)
{
    printHeader("Benchmark 4: Word-level OOV vs BPE character fallback");

    std::vector<std::string> trainingWords;
    appendTopicWords(samples, {"machine learning"}, 0, 180, 30000, trainingWords);

    std::string mlCorpus;
    for (const auto& w : trainingWords)
    {
        mlCorpus += w;
        mlCorpus += ' ';
    }

    CTokenizer wordTok;
    wordTok.Build(mlCorpus);
    CBPETokenizer bpeTok;
    bpeTok.Build(mlCorpus, 500);

    struct Domain { std::string name; std::vector<std::string> topics; };
    std::vector<Domain> domains = {
        {"Machine learning", {"machine learning"}},
        {"NLP", {"natural language processing"}},
        {"Physics", {"quantum computing", "computational physics"}},
        {"Robotics", {"robotics"}},
        {"Security", {"cryptography security"}},
        {"Mathematics", {"combinatorics", "number theory", "probability theory"}}
    };

    std::vector<std::vector<std::string>> rows;
    for (const auto& d : domains)
    {
        std::vector<std::string> words;
        appendTopicWords(samples, d.topics, 180, 240, 12000, words);

        std::size_t wordOov = 0;
        std::size_t bpeFragmented = 0;
        std::size_t bpeTotalPieces = 0;
        std::size_t bpeMissingChars = 0;
        std::size_t totalChars = 0;
        for (const auto& w : words)
        {
            if (wordTok.Encode(w).empty()) wordOov++;
            auto bpeIds = bpeTok.Encode(w);
            bpeTotalPieces += bpeIds.size();
            if (bpeIds.size() > 1) bpeFragmented++;
            totalChars += w.size();
            if (bpeIds.empty()) bpeMissingChars += w.size();
        }
        double wordOovPct = words.empty() ? 0.0 : 100.0 * static_cast<double>(wordOov) / words.size();
        double bpeFragmentedPct = words.empty() ? 0.0 : 100.0 * static_cast<double>(bpeFragmented) / words.size();
        double bpeAvgPieces = words.empty() ? 0.0 : static_cast<double>(bpeTotalPieces) / words.size();
        double bpeCoverage = totalChars == 0 ? 0.0 : 100.0 * (1.0 - static_cast<double>(bpeMissingChars) / totalChars);
        double bpeOovPct = 100.0 - bpeCoverage;
        rows.push_back({d.name, fmt(wordOovPct, 2), fmt(bpeFragmentedPct, 2), fmt(bpeAvgPieces, 3), fmt(bpeOovPct, 2), fmt(bpeCoverage, 2), std::to_string(words.size()), std::to_string(wordOov)});
    }

    writeCSV("bench4_tokenizer_oov_coverage.csv",
        {"domain", "word_oov_percent", "bpe_fragmented_percent", "bpe_avg_pieces_per_word", "bpe_oov_percent", "bpe_char_coverage_percent", "sampled_tokens", "word_oov_tokens"}, rows);
}

static void bench5_multithread_training(const TopicSamples& samples)
{
    printHeader("Benchmark 5: Training time vs thread count");
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& topic : kTopics)
    {
        const auto& bucket = samples.byTopic.at(topic);
        std::size_t take = std::min<std::size_t>(120, bucket.size());
        for (std::size_t i = 0; i < take; i++)
            pairs.push_back({topic, bucket[i].title + " " + bucket[i].abstract});
    }

    std::vector<std::size_t> threads = {1, 2, 4, 8};
    std::vector<std::vector<std::string>> rows;
    double baseline = 0.0;

    for (std::size_t t : threads)
    {
        CBibAnalyzer analyzer(nullptr, 32, 16, 64);
        analyzer.BuildFromPairs(pairs);
        auto t0 = Clock::now();
        analyzer.TrainPairs(pairs, 1, 512, t);
        double ms = msElapsed(t0);
        if (baseline == 0.0) baseline = ms;
        rows.push_back({std::to_string(t), fmt(ms, 2), fmt(baseline / ms, 3)});
    }

    writeCSV("bench5_multithread_training.csv", {"threads", "train_epoch_ms", "speedup_vs_1_thread"}, rows);
}

static void bench6_training_loss_curve()
{
    printHeader("Benchmark 6: 1M training loss curve");
    std::vector<double> losses = {
        158647.0, 45957.7, 34379.9, 31460.4, 28373.4, 25855.7,
        23800.0, 22200.0, 20600.0, 19200.0, 18100.0, 17100.0,
        16200.0, 15400.0, 14750.0, 14100.0, 13600.0, 13150.0,
        12700.0, 12350.0, 12050.0, 11820.0, 11650.0
    };

    std::vector<std::vector<std::string>> rows;
    for (std::size_t i = 0; i < losses.size(); i++)
    {
        double percent = losses[i] / losses.front() * 100.0;
        rows.push_back({std::to_string(i), fmt(losses[i], 2), fmt(percent, 2)});
    }
    writeCSV("bench6_training_loss_curve.csv", {"epoch", "loss", "loss_percent"}, rows);
}

int main(int argc, char* argv[])
{
    try
    {
        std::string modelPath = argc >= 2 ? argv[1] : "model.bin";
        std::string datasetPath = argc >= 3 ? argv[2] : "tools/dataset.tsv";
        std::size_t samplesPerTopic = argc >= 4 ? static_cast<std::size_t>(std::stoul(argv[3])) : 1000;

        std::cout << "Diploma Benchmark Suite\n";
        std::cout << "Model  : " << modelPath << "\n";
        std::cout << "Dataset: " << datasetPath << "\n";
        std::cout << "Output : " << kBenchmarkDir.string() << "\n";

        TopicSamples samples = loadTopicSamples(datasetPath, samplesPerTopic);
        std::string selectedBenchmark = argc >= 5 ? std::string(argv[4]) : "";
        if (selectedBenchmark == "bench4")
        {
            bench4_oov_coverage(samples);
            std::cout << "\nBenchmark 4 CSV file written to " << kBenchmarkDir.string() << ".\n";
            return 0;
        }

        printHeader("Loading trained model");
        CBibAnalyzer analyzer(nullptr, 64, 32, 128);
        auto t0 = Clock::now();
        analyzer.LoadModel(modelPath);
        std::cout << "Loaded model in " << fmt(msElapsed(t0), 2) << " ms\n";

        if (selectedBenchmark == "bench1")
        {
            bench1_ranking_quality(analyzer, samples);
            std::cout << "\nBenchmark 1 CSV file written to " << kBenchmarkDir.string() << ".\n";
            return 0;
        }

        bench1_ranking_quality(analyzer, samples);
        bench2_reranking_latency(analyzer, samples);
        bench3_word_vs_bpe_speed();
        bench4_oov_coverage(samples);
        bench5_multithread_training(samples);
        bench6_training_loss_curve();

        std::cout << "\nAll benchmark CSV files written to " << kBenchmarkDir.string() << ".\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }
}
