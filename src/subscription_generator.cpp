#include "subscription_generator.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <thread>

// =====================================================
// Varianta simpla - o pastram, dar nu va fi folosita
// in forma finala a temei
// =====================================================

static bool shouldIncludeField(std::mt19937& rng, double percentage) {
    std::uniform_real_distribution<double> dist(0.0, 100.0);
    return dist(rng) < percentage;
}

static OperatorType randomStringOperator(std::mt19937& rng, bool forceEquality = false) {
    if (forceEquality) {
        return OperatorType::EQ;
    }

    std::vector<OperatorType> ops = {
        OperatorType::EQ,
        OperatorType::NEQ
    };

    int index = randomInt(rng, 0, static_cast<int>(ops.size()) - 1);
    return ops[index];
}

static OperatorType randomNumericOperator(std::mt19937& rng) {
    std::vector<OperatorType> ops = {
        OperatorType::EQ,
        OperatorType::NEQ,
        OperatorType::LT,
        OperatorType::LE,
        OperatorType::GT,
        OperatorType::GE
    };

    int index = randomInt(rng, 0, static_cast<int>(ops.size()) - 1);
    return ops[index];
}

Subscription generateSubscription(const Config& config, std::mt19937& rng) {
    Subscription sub;

    bool hasCompany = shouldIncludeField(rng, config.companyFreqPct);
    bool hasValue = shouldIncludeField(rng, config.valueFreqPct);
    bool hasDrop = shouldIncludeField(rng, config.dropFreqPct);
    bool hasVariation = shouldIncludeField(rng, config.variationFreqPct);
    bool hasDate = shouldIncludeField(rng, config.dateFreqPct);

    if (!hasCompany && !hasValue && !hasDrop && !hasVariation && !hasDate) {
        hasCompany = true;
    }

    if (hasCompany) {
        SubscriptionField field;
        field.fieldType = FieldType::COMPANY;
        field.op = randomStringOperator(rng);
        field.stringValue = randomChoice(rng, config.companies);
        sub.fields.push_back(field);
    }

    if (hasValue) {
        SubscriptionField field;
        field.fieldType = FieldType::VALUE;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subValueMin, config.subValueMax);
        sub.fields.push_back(field);
    }

    if (hasDrop) {
        SubscriptionField field;
        field.fieldType = FieldType::DROP;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subDropMin, config.subDropMax);
        sub.fields.push_back(field);
    }

    if (hasVariation) {
        SubscriptionField field;
        field.fieldType = FieldType::VARIATION;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subVariationMin, config.subVariationMax);
        sub.fields.push_back(field);
    }

    if (hasDate) {
        SubscriptionField field;
        field.fieldType = FieldType::DATE;
        field.op = randomStringOperator(rng);
        field.stringValue = randomChoice(rng, config.dates);
        sub.fields.push_back(field);
    }

    return sub;
}

std::vector<Subscription> generateSubscriptionsSimple(const Config& config) {
    std::vector<Subscription> subscriptions;
    subscriptions.reserve(config.numSubscriptions);

    std::mt19937 rng(std::random_device{}());

    for (size_t i = 0; i < config.numSubscriptions; i++) {
        subscriptions.push_back(generateSubscription(config, rng));
    }

    return subscriptions;
}

// =====================================================
// Varianta finala - distributie controlata
// =====================================================

struct SubscriptionPlan {
    bool hasCompany = false;
    bool hasValue = false;
    bool hasDrop = false;
    bool hasVariation = false;
    bool hasDate = false;

    OperatorType companyOp = OperatorType::EQ;
};

static size_t percentToCount(size_t total, double percentage) {
    return static_cast<size_t>(std::llround((percentage / 100.0) * static_cast<double>(total)));
}

static void assignFieldToPlans(std::vector<SubscriptionPlan>& plans,
                               size_t count,
                               FieldType field,
                               std::mt19937& rng) {
    std::vector<size_t> indices(plans.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    count = std::min(count, plans.size());

    for (size_t i = 0; i < count; i++) {
        size_t idx = indices[i];

        switch (field) {
            case FieldType::COMPANY:
                plans[idx].hasCompany = true;
                break;
            case FieldType::VALUE:
                plans[idx].hasValue = true;
                break;
            case FieldType::DROP:
                plans[idx].hasDrop = true;
                break;
            case FieldType::VARIATION:
                plans[idx].hasVariation = true;
                break;
            case FieldType::DATE:
                plans[idx].hasDate = true;
                break;
        }
    }
}

static void ensureNoEmptySubscriptions(std::vector<SubscriptionPlan>& plans, std::mt19937& rng) {
    for (auto& plan : plans) {
        bool isEmpty = !plan.hasCompany &&
                       !plan.hasValue &&
                       !plan.hasDrop &&
                       !plan.hasVariation &&
                       !plan.hasDate;

        if (isEmpty) {
            // alegem un camp random de fallback
            int choice = randomInt(rng, 0, 4);
            switch (choice) {
                case 0: plan.hasCompany = true; break;
                case 1: plan.hasValue = true; break;
                case 2: plan.hasDrop = true; break;
                case 3: plan.hasVariation = true; break;
                case 4: plan.hasDate = true; break;
            }
        }
    }
}

static void assignCompanyOperators(std::vector<SubscriptionPlan>& plans,
                                   const Config& config,
                                   std::mt19937& rng) {
    std::vector<size_t> companyIndices;

    for (size_t i = 0; i < plans.size(); i++) {
        if (plans[i].hasCompany) {
            companyIndices.push_back(i);
        }
    }

    if (companyIndices.empty()) {
        return;
    }

    std::shuffle(companyIndices.begin(), companyIndices.end(), rng);

    size_t minEqCount = percentToCount(companyIndices.size(), config.companyEqMinPct);
    minEqCount = std::min(minEqCount, companyIndices.size());

    for (size_t i = 0; i < companyIndices.size(); i++) {
        size_t idx = companyIndices[i];

        if (i < minEqCount) {
            plans[idx].companyOp = OperatorType::EQ;
        } else {
            plans[idx].companyOp = OperatorType::NEQ;
        }
    }
}

static Subscription buildSubscriptionFromPlan(const SubscriptionPlan& plan,
                                              const Config& config,
                                              std::mt19937& rng) {
    Subscription sub;

    if (plan.hasCompany) {
        SubscriptionField field;
        field.fieldType = FieldType::COMPANY;
        field.op = plan.companyOp;
        field.stringValue = randomChoice(rng, config.companies);
        sub.fields.push_back(field);
    }

    if (plan.hasValue) {
        SubscriptionField field;
        field.fieldType = FieldType::VALUE;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subValueMin, config.subValueMax);
        sub.fields.push_back(field);
    }

    if (plan.hasDrop) {
        SubscriptionField field;
        field.fieldType = FieldType::DROP;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subDropMin, config.subDropMax);
        sub.fields.push_back(field);
    }

    if (plan.hasVariation) {
        SubscriptionField field;
        field.fieldType = FieldType::VARIATION;
        field.op = randomNumericOperator(rng);
        field.numericValue = randomDouble(rng, config.subVariationMin, config.subVariationMax);
        sub.fields.push_back(field);
    }

    if (plan.hasDate) {
        SubscriptionField field;
        field.fieldType = FieldType::DATE;
        field.op = randomStringOperator(rng);
        field.stringValue = randomChoice(rng, config.dates);
        sub.fields.push_back(field);
    }

    return sub;
}

static std::vector<SubscriptionPlan> buildBalancedPlans(const Config& config, std::mt19937& rng) {
    std::vector<SubscriptionPlan> plans(config.numSubscriptions);

    size_t companyCount = percentToCount(config.numSubscriptions, config.companyFreqPct);
    size_t valueCount = percentToCount(config.numSubscriptions, config.valueFreqPct);
    size_t dropCount = percentToCount(config.numSubscriptions, config.dropFreqPct);
    size_t variationCount = percentToCount(config.numSubscriptions, config.variationFreqPct);
    size_t dateCount = percentToCount(config.numSubscriptions, config.dateFreqPct);

    assignFieldToPlans(plans, companyCount, FieldType::COMPANY, rng);
    assignFieldToPlans(plans, valueCount, FieldType::VALUE, rng);
    assignFieldToPlans(plans, dropCount, FieldType::DROP, rng);
    assignFieldToPlans(plans, variationCount, FieldType::VARIATION, rng);
    assignFieldToPlans(plans, dateCount, FieldType::DATE, rng);

    ensureNoEmptySubscriptions(plans, rng);
    assignCompanyOperators(plans, config, rng);

    return plans;
}

std::vector<Subscription> generateSubscriptionsBalanced(const Config& config) {
    std::vector<Subscription> subscriptions;
    subscriptions.reserve(config.numSubscriptions);

    std::mt19937 rng(std::random_device{}());
    std::vector<SubscriptionPlan> plans = buildBalancedPlans(config, rng);

    for (const auto& plan : plans) {
        subscriptions.push_back(buildSubscriptionFromPlan(plan, config, rng));
    }

    return subscriptions;
}

std::vector<Subscription> generateSubscriptionsBalancedParallel(const Config& config, size_t numThreads) {
    if (numThreads <= 1 || config.numSubscriptions == 0) {
        return generateSubscriptionsBalanced(config);
    }

    std::mt19937 planRng(std::random_device{}());
    std::vector<SubscriptionPlan> plans = buildBalancedPlans(config, planRng);

    std::vector<Subscription> subscriptions(config.numSubscriptions);

    size_t workerCount = std::min(numThreads, config.numSubscriptions);
    size_t baseChunk = config.numSubscriptions / workerCount;
    size_t remainder = config.numSubscriptions % workerCount;

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    size_t start = 0;
    for (size_t worker = 0; worker < workerCount; worker++) {
        size_t chunk = baseChunk + (worker < remainder ? 1 : 0);
        size_t end = start + chunk;

        workers.emplace_back([&, start, end, worker]() {
            std::mt19937 rng(std::random_device{}() + 1000u + static_cast<unsigned int>(worker));
            for (size_t i = start; i < end; i++) {
                subscriptions[i] = buildSubscriptionFromPlan(plans[i], config, rng);
            }
        });

        start = end;
    }

    for (auto& worker : workers) {
        worker.join();
    }

    return subscriptions;
}