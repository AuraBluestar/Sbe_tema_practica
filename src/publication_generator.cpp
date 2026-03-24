#include "publication_generator.h"
#include "utils.h"

#include <algorithm>
#include <thread>
#include <vector>

Publication generatePublication(const Config& config, std::mt19937& rng) {
    Publication pub;

    // stringuri din seturi prestabilite
    pub.company = randomChoice(rng, config.companies);
    pub.date = randomChoice(rng, config.dates);

    // valori numerice din intervale configurate
    pub.value = randomDouble(rng, config.pubValueMin, config.pubValueMax);
    pub.drop = randomDouble(rng, config.pubDropMin, config.pubDropMax);
    pub.variation = randomDouble(rng, config.pubVariationMin, config.pubVariationMax);

    return pub;
}

std::vector<Publication> generatePublicationsSequential(const Config& config) {
    std::vector<Publication> publications;
    publications.reserve(config.numPublications);

    std::mt19937 rng(std::random_device{}());
    for (size_t i = 0; i < config.numPublications; i++) {
        publications.push_back(generatePublication(config, rng));
    }

    return publications;
}

std::vector<Publication> generatePublicationsParallel(const Config& config, size_t numThreads) {
    if (numThreads <= 1 || config.numPublications == 0) {
        return generatePublicationsSequential(config);
    }

    std::vector<Publication> publications(config.numPublications);

    size_t workerCount = std::min(numThreads, config.numPublications);
    size_t baseChunk = config.numPublications / workerCount;
    size_t remainder = config.numPublications % workerCount;

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    size_t start = 0;
    for (size_t worker = 0; worker < workerCount; worker++) {
        size_t chunk = baseChunk + (worker < remainder ? 1 : 0);
        size_t end = start + chunk;

        workers.emplace_back([&, start, end, worker]() {
            std::mt19937 rng(std::random_device{}() + static_cast<unsigned int>(worker));
            for (size_t i = start; i < end; i++) {
                publications[i] = generatePublication(config, rng);
            }
        });

        start = end;
    }

    for (auto& worker : workers) {
        worker.join();
    }

    return publications;
}