#include "biblio/CArxivSource.h"
#include "biblio/CBibAnalyzer.h"

#include <iostream>
#include <string>
#include <vector>

static void printUsage(const char* exe)
{
    std::cout
        << "Usage: " << exe << " [options]\n\n"
        << "  API mode (fetch from Arxiv):\n"
        << "    --topics t1 t2 ...  Topics to fetch (default: machine learning, deep learning, ...)\n\n"
        << "  Dataset mode (local file):\n"
        << "    --dataset path      TSV file: query<TAB>title<TAB>abstract\n"
        << "                        or:       query<TAB>text\n\n"
        << "  Common options:\n"
        << "    --epochs N          Training epochs (default: 5 in dataset mode, 20 in API mode)\n"
        << "    --max-pairs N       Max dataset pairs to sample (default: 12000, 0 = all)\n"
        << "    --batch-size N      Mini-batch size (default: 1024)\n"
        << "    --threads N         Training worker threads (default: hardware concurrency)\n"
        << "    --output path       Output model file (default: model.bin)\n"
        << "    --dmodel N          Embedding dimension (default: 64)\n"
        << "    --dk N              Attention key dimension (default: 32)\n"
        << "    --dff N             Feed-forward hidden size (default: 128)\n";
}

int main(int argc, char* argv[])
{
    std::vector<std::string> topics = {
        "machine learning",
        "deep learning",
        "neural networks",
        "natural language processing",
        "transformer attention"
    };

    std::string dataset;
    std::size_t epochs = 0;
    std::size_t maxPairs = 12000;
    std::size_t batchSize = 1024;
    std::size_t numThreads = 0;
    std::string output = "model.bin";
    std::size_t dModel = 64;
    std::size_t dK     = 32;
    std::size_t dFF    = 128;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--dataset" && i + 1 < argc) dataset = argv[++i];
        else if (arg == "--epochs" && i + 1 < argc) epochs = std::stoul(argv[++i]);
        else if (arg == "--max-pairs" && i + 1 < argc) maxPairs = std::stoul(argv[++i]);
        else if (arg == "--batch-size" && i + 1 < argc) batchSize = std::stoul(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) numThreads = std::stoul(argv[++i]);
        else if (arg == "--output" && i + 1 < argc) output = argv[++i];
        else if (arg == "--dmodel" && i + 1 < argc) dModel = std::stoul(argv[++i]);
        else if (arg == "--dk" && i + 1 < argc) dK = std::stoul(argv[++i]);
        else if (arg == "--dff" && i + 1 < argc) dFF = std::stoul(argv[++i]);
        else if (arg == "--topics")
        {
            topics.clear();
            while (i + 1 < argc && argv[i + 1][0] != '-')
                topics.push_back(argv[++i]);
        }
    }

    if (epochs == 0)
        epochs = dataset.empty() ? 20 : 5;

    std::cout << "=== Training configuration ===\n";
    std::cout << "Mode     : " << (dataset.empty() ? "Arxiv API" : "Dataset file") << "\n";
    if (!dataset.empty())
    {
        std::cout << "Dataset  : " << dataset << "\n";
        std::cout << "MaxPairs : " << (maxPairs == 0 ? std::string("all") : std::to_string(maxPairs)) << "\n";
    }
    else
    {
        std::cout << "Topics   : ";
        for (const auto& t : topics) std::cout << "\"" << t << "\" ";
        std::cout << "\n";
    }
    std::cout << "Epochs   : " << epochs << "\n";
    std::cout << "Batch    : " << batchSize << "\n";
    std::cout << "Threads  : " << (numThreads == 0 ? std::string("auto") : std::to_string(numThreads)) << "\n";
    std::cout << "dModel   : " << dModel << "\n";
    std::cout << "dK       : " << dK     << "\n";
    std::cout << "dFF      : " << dFF    << "\n";
    std::cout << "Output   : " << output << "\n\n";

    CArxiveSource source;
    CBibAnalyzer analyzer(&source, dModel, dK, dFF);

    if (!dataset.empty())
    {
        auto pairs = analyzer.LoadDataset(dataset, maxPairs);
        if (pairs.empty())
        {
            std::cerr << "Dataset is empty or could not be read.\n";
            return 1;
        }

        std::cout << "\n=== Building vocabulary ===\n";
        analyzer.BuildFromPairs(pairs);

        std::cout << "\n=== Training ===\n";
        analyzer.TrainPairs(pairs, epochs, batchSize, numThreads);
    }
    else
    {
        std::cout << "=== Fetching articles from Arxiv ===\n";
        analyzer.Build(topics);

        std::cout << "\n=== Training ===\n";
        analyzer.Train(topics, epochs);
    }

    std::cout << "\n=== Saving model to " << output << " ===\n";
    analyzer.SaveModel(output);
    std::cout << "Done.\n";

    return 0;
}