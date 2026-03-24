#pragma once

#include <random>
#include <vector>
#include "models.h"

Publication generatePublication(const Config& config, std::mt19937& rng);
std::vector<Publication> generatePublicationsSequential(const Config& config);
std::vector<Publication> generatePublicationsParallel(const Config& config, size_t numThreads);