#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

#include "models.h"
#include "publication_generator.h"
#include "subscription_generator.h"
#include "writer.h"

struct RunStats {
    size_t publications = 0;
    size_t subscriptions = 0;
    double generationSec = 0.0;
    double writeSec = 0.0;
    double totalSec = 0.0;
};

static RunStats runScenario(const Config& baseConfig,
                            size_t numThreads,
                            bool writeFiles,
                            const std::string& outputPrefix = "") {
    Config config = baseConfig;
    config.numThreads = numThreads;

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Publication> publications = generatePublicationsParallel(config, config.numThreads);
    std::vector<Subscription> subscriptions =
        generateSubscriptionsBalancedParallel(config, config.numThreads);

    auto afterGeneration = std::chrono::high_resolution_clock::now();

    if (writeFiles) {
        std::filesystem::create_directories("output");
        const std::string pubFile = outputPrefix + "publications.txt";
        const std::string subFile = outputPrefix + "subscriptions.txt";
        writePublicationsToFile(pubFile, publications);
        writeSubscriptionsToFile(subFile, subscriptions);
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> generationTime = afterGeneration - start;
    std::chrono::duration<double> writeTime = end - afterGeneration;
    std::chrono::duration<double> totalTime = end - start;

    RunStats stats;
    stats.publications = publications.size();
    stats.subscriptions = subscriptions.size();
    stats.generationSec = generationTime.count();
    stats.writeSec = writeTime.count();
    stats.totalSec = totalTime.count();
    return stats;
}

int main() {
    Config config;

    config.numPublications = 200000;
    config.numSubscriptions = 200000;

    std::cout << "=== Benchmark (fara scriere in fisier) ===\n";
    for (size_t threads : {1u, 4u}) {
        RunStats stats = runScenario(config, threads, false);
        std::cout << "Threads: " << threads
                  << " | Publ: " << stats.publications
                  << " | Subs: " << stats.subscriptions
                  << " | Generare: " << stats.generationSec << " sec"
                  << " | Total: " << stats.totalSec << " sec\n";
    }

    std::cout << "\n=== Generare finala cu scriere in output/ ===\n";
    RunStats finalStats = runScenario(config, 4, true, "output/");

    std::cout << "Publicatii generate   : " << finalStats.publications << "\n";
    std::cout << "Subscriptii generate  : " << finalStats.subscriptions << "\n";
    std::cout << "Timp generare         : " << finalStats.generationSec << " sec\n";
    std::cout << "Timp scriere fisiere  : " << finalStats.writeSec << " sec\n";
    std::cout << "Timp total            : " << finalStats.totalSec << " sec\n";

    return 0;
}