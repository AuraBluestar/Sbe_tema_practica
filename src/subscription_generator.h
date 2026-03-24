#pragma once

#include <random>
#include <vector>
#include "models.h"

Subscription generateSubscription(const Config& config, std::mt19937& rng);
std::vector<Subscription> generateSubscriptionsSimple(const Config& config);

// varianta finala, cu distributie controlata
std::vector<Subscription> generateSubscriptionsBalanced(const Config& config);
std::vector<Subscription> generateSubscriptionsBalancedParallel(const Config& config, size_t numThreads);