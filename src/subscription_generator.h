#pragma once

#include <random>
#include <vector>
#include "models.h"


std::vector<Subscription> generateSubscriptionsBalanced(const Config& config);
std::vector<Subscription> generateSubscriptionsBalancedParallel(const Config& config, size_t numThreads);