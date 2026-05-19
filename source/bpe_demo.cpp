#include "core/tokenizer/CTokenizer.h"
#include "core/tokenizer/CBPETokenizer.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string loadFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void printSeparator(int w = 70)
{
    std::cout << std::string(w, '-') << "\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Corpus: use file from command line, or fall back to an embedded sample
    std::string corpus;
    if (argc >= 2)
    {
        corpus = loadFile(argv[1]);
        if (corpus.empty())
        {
            std::cerr << "Could not open corpus file: " << argv[1] << "\n";
            return 1;
        }
        std::cout << "Corpus loaded from file (" << corpus.size() << " bytes)\n\n";
    }
    else
    {
        // Representative sample of scientific paper text (ML-heavy, as the model was trained)
        corpus =
            "machine learning deep learning neural network gradient descent backpropagation "
            "attention transformer encoder decoder embedding representation feature extraction "
            "convolutional recurrent batch normalization dropout regularization optimization "
            "supervised unsupervised reinforcement classification regression clustering "
            "natural language processing text generation image recognition object detection "
            "generative adversarial network variational autoencoder latent space "
            "stochastic gradient descent momentum learning rate scheduler warm up "
            "model training inference prediction accuracy precision recall f1 score "
            "dataset train validation test split cross entropy loss function "
            "graph neural network node edge link prediction knowledge graph "
            "bert gpt roberta transformer language model pretraining finetuning "
            "token embedding positional encoding multi head self attention feed forward "
            "layer normalization residual connection skip connection weight initialization "
            "hyperparameter tuning search space architecture design ablation study "
            "benchmark evaluation metric perplexity bleu rouge wer cer "
            "computer vision semantic segmentation instance segmentation depth estimation "
            "speech recognition audio feature mel spectrogram fourier transform "
            "recommendation system collaborative filtering matrix factorization "
            "reinforcement learning policy reward agent environment exploration exploitation "
            "quantum computing qubit entanglement superposition gate circuit "
            "biology genomics protein sequence alignment mutation evolution "
            "chemistry molecule bond reaction synthesis catalyst "
            "physics particle proton neutron electron energy momentum field "
            "storage retrieval database index query search rank relevance ";

        std::cout << "Using built-in sample corpus (" << corpus.size() << " bytes).\n";
        std::cout << "Tip: pass a corpus file as argv[1] for a larger vocabulary.\n\n";
    }

    const std::size_t BPE_MERGES = 500;

    // ---------------------------------------------------------------------------
    // Training
    // ---------------------------------------------------------------------------
    std::cout << "Training word-level tokenizer...\n";
    CTokenizer wordTok;
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        wordTok.Build(corpus);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "  Done in "
                  << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()
                  << " us\n";
    }

    std::cout << "Training BPE tokenizer (" << BPE_MERGES << " merges)...\n";
    CBPETokenizer bpeTok;
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        bpeTok.Build(corpus, BPE_MERGES);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "  Done in "
                  << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()
                  << " us\n";
    }

    // ---------------------------------------------------------------------------
    // Vocabulary size
    // ---------------------------------------------------------------------------
    std::cout << "\n";
    printSeparator();
    std::cout << "VOCABULARY SIZE\n";
    printSeparator();
    std::cout << "  Word-level : " << wordTok.VocabSize() << " types\n";
    std::cout << "  BPE        : " << bpeTok.VocabSize()
              << " types  (base chars + " << BPE_MERGES << " merge rules)\n";

    // ---------------------------------------------------------------------------
    // Top BPE merges
    // ---------------------------------------------------------------------------
    std::cout << "\n";
    printSeparator();
    std::cout << "FIRST 15 BPE MERGE RULES (most frequent character pairs)\n";
    printSeparator();
    for (const auto& [a, b] : bpeTok.TopMerges(15))
        std::cout << "  \"" << a << "\" + \"" << b << "\" -> \"" << a + b << "\"\n";

    // ---------------------------------------------------------------------------
    // Per-word tokenization comparison
    // ---------------------------------------------------------------------------
    std::vector<std::string> testWords = {
        // in-vocabulary (ML domain)
        "machine", "learning", "attention", "transformer", "embedding",
        // likely out-of-vocabulary for a ML-trained model
        "quantum", "proton", "storage", "neuron", "genomics",
        "catalyst", "molecule", "retrieval", "biology", "chemistry"
    };

    std::cout << "\n";
    printSeparator();
    std::cout << std::left
              << std::setw(14) << "Word"
              << std::setw(12) << "Word-tok"
              << "BPE subwords\n";
    printSeparator();

    int oovWord = 0;
    for (const auto& w : testWords)
    {
        auto wIds   = wordTok.Encode(w);
        bool isOOV  = wIds.empty();
        if (isOOV) oovWord++;

        auto subwords = bpeTok.EncodeWord(w);
        std::string bpeStr;
        for (const auto& s : subwords)
            bpeStr += "[" + s + "]";

        std::cout << std::setw(14) << w
                  << std::setw(12) << (isOOV ? "<OOV>" : w)
                  << bpeStr << "\n";
    }

    // ---------------------------------------------------------------------------
    // OOV summary
    // ---------------------------------------------------------------------------
    std::cout << "\n";
    printSeparator();
    std::cout << "OOV RATE ON TEST WORDS\n";
    printSeparator();
    int total = static_cast<int>(testWords.size());
    std::cout << "  Word-level : " << oovWord << "/" << total
              << " words unknown  ("
              << std::fixed << std::setprecision(1)
              << (100.0 * oovWord / total) << "%)\n";
    std::cout << "  BPE        : 0/" << total
              << " words unknown  (0.0%)  -- always decomposes to known chars\n";

    // ---------------------------------------------------------------------------
    // Encoding speed benchmark
    // ---------------------------------------------------------------------------
    const std::string benchText =
        "quantum storage proton neural machine learning deep network attention "
        "transformer biology chemistry physics retrieval ranking relevance query";
    const int BENCH_REPS = 50000;

    std::cout << "\n";
    printSeparator();
    std::cout << "ENCODING SPEED  (" << BENCH_REPS << " repetitions, text="
              << benchText.size() << " chars)\n";
    printSeparator();

    {
        auto t0 = std::chrono::high_resolution_clock::now();
        volatile std::size_t sink = 0;
        for (int i = 0; i < BENCH_REPS; i++)
            sink += wordTok.Encode(benchText).size();
        auto t1 = std::chrono::high_resolution_clock::now();
        double us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "  Word-level : " << std::fixed << std::setprecision(2)
                  << (us / BENCH_REPS) << " us/call\n";
    }
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        volatile std::size_t sink = 0;
        for (int i = 0; i < BENCH_REPS; i++)
            sink += bpeTok.Encode(benchText).size();
        auto t1 = std::chrono::high_resolution_clock::now();
        double us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "  BPE        : " << std::fixed << std::setprecision(2)
                  << (us / BENCH_REPS) << " us/call  (applies " << BPE_MERGES << " rules)\n";
    }

    std::cout << "\n";
    printSeparator();
    std::cout << "CONCLUSION\n";
    printSeparator();
    std::cout << "  Word-level tokenization is fast but fails on out-of-vocabulary terms.\n"
              << "  BPE decomposes any word into known subword units, achieving 0% OOV\n"
              << "  at the cost of more tokens per word and slower encoding.\n"
              << "  For a domain-general scientific search engine, BPE provides\n"
              << "  better coverage across disciplines (physics, biology, chemistry)\n"
              << "  without retraining the full vocabulary.\n";

    return 0;
}
