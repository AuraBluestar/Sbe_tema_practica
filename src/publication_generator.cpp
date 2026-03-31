#include "publication_generator.h"
#include "utils.h"

#include <vector>
#include <thread>
#include <random>

// Varianta secventiala
std::vector<Publication> generatePublicationsSequential(const Config& config) {
    int N = (int)config.numPublications;

    std::vector<Publication> pubs;
    pubs.reserve(N);

    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < N; i++) {
        Publication p;

        p.company = randomChoice(rng, config.companies);
        p.date = randomChoice(rng, config.dates);

        p.value = randomDouble(rng, config.pubValueMin, config.pubValueMax);
        p.drop = randomDouble(rng, config.pubDropMin, config.pubDropMax);
        p.variation = randomDouble(rng, config.pubVariationMin, config.pubVariationMax);

        pubs.push_back(p);
    }

    return pubs;
}

// Varianta paralela
std::vector<Publication> generatePublicationsParallel(const Config& config, size_t numThreads) {
    int N = (int)config.numPublications;

    if (numThreads <= 1 || N == 0) {
        return generatePublicationsSequential(config);
    }

    std::vector<Publication> pubs(N);

    int chunk = N / (int)numThreads;
    std::vector<std::thread> threads;

    for (int t = 0; t < (int)numThreads; t++) {
        int start = t * chunk;
        int end = (t == (int)numThreads - 1) ? N : start + chunk;

        threads.emplace_back([&, start, end]() {
            std::mt19937 rng(std::random_device{}());

            for (int i = start; i < end; i++) {
                Publication p;

                p.company = randomChoice(rng, config.companies);
                p.date = randomChoice(rng, config.dates);

                p.value = randomDouble(rng, config.pubValueMin, config.pubValueMax);
                p.drop = randomDouble(rng, config.pubDropMin, config.pubDropMax);
                p.variation = randomDouble(rng, config.pubVariationMin, config.pubVariationMax);

                pubs[i] = p;
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    return pubs;
}