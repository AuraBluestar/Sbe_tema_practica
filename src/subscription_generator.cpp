#include "subscription_generator.h"
#include "utils.h"

#include <vector>
#include <algorithm>
#include <thread>
#include <random>
#include <iostream>

void printStats(const std::vector<Subscription>& subs) {
    int total = subs.size();
    int companyCount = 0;
    int eqCount = 0;
    int dateCount=0;
    int variationCount=0;
    int dropCount=0;int valueCount=0;

    for (const auto& s : subs) {
        if(s.fields.size()==0) std::cout<<"o subs e goala";
        for (const auto& f : s.fields) {
            if (f.fieldType == FieldType::COMPANY) {
                companyCount++;
                if (f.op == OperatorType::EQ)
                    eqCount++;
            }
            else if(f.fieldType== FieldType::DATE)
            dateCount++;
            else if(f.fieldType== FieldType::VARIATION)
            variationCount++;
             else if(f.fieldType== FieldType::DROP)
            dropCount++;
            else if(f.fieldType== FieldType::VALUE)
            valueCount++;
            
        }
    }

    std::cout << "\n=== STATISTICI ===\n";
    std::cout << "Company presence: " << (100.0 * companyCount / total) << "%\n";
     std::cout << "Value presence: " << (100.0 * valueCount / total) << "%\n";
      std::cout << "Drop presence: " << (100.0 * dropCount / total) << "%\n";
       std::cout << "Date presence: " << (100.0 * dateCount / total) << "%\n";
        std::cout << "Variation presence: " << (100.0 * variationCount / total) << "%\n";
    std::cout << "Company EQ rate: " << (100.0 * eqCount / companyCount) << "%\n";
   
}

// ce contine fiecare subscriptie
struct Plan {
    bool hasCompany = false;
    bool hasValue = false;
    bool hasDrop = false;
    bool hasVariation = false;
    bool hasDate = false;

    OperatorType companyOp = OperatorType::EQ;
};

static std::vector<Plan> buildPlan(const Config& config) {
    int N = (int)config.numSubscriptions;
    std::vector<Plan> plans(N);
    std::mt19937 rng(std::random_device{}());

    int nCompany   = N * config.companyFreqPct   / 100;
    int nValue     = N * config.valueFreqPct     / 100;
    int nDrop      = N * config.dropFreqPct      / 100;
    int nVariation = N * config.variationFreqPct / 100;
    int nDate      = N * config.dateFreqPct      / 100;

    std::vector<int> indices(N);
    for (int i = 0; i < N; i++) indices[i] = i;

    // Distribuim toate câmpurile normal prin shuffle
    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < nCompany; i++)
        plans[indices[i]].hasCompany = true;

    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < nValue; i++)
        plans[indices[i]].hasValue = true;

    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < nDrop; i++)
        plans[indices[i]].hasDrop = true;

    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < nVariation; i++)
        plans[indices[i]].hasVariation = true;

    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < nDate; i++)
        plans[indices[i]].hasDate = true;

     // reparam subs goale - luam indicii cu company care au SI alte campuri (pot ceda company)
    std::vector<int> potCedaCompany;
    for (int i = 0; i < N; i++) {
        if (plans[i].hasCompany &&
            (plans[i].hasValue || plans[i].hasDrop ||
             plans[i].hasVariation || plans[i].hasDate)) {
            potCedaCompany.push_back(i);
        }
    }

    int cedatDinCompany = 0;

    for (int i = 0; i < N; i++) {
        if (!plans[i].hasCompany && !plans[i].hasValue && !plans[i].hasDrop
            && !plans[i].hasVariation && !plans[i].hasDate) {

            // orice subscriptie goala- primeste un company de la cineva
            plans[i].hasCompany = true;

            // Luam company de la cineva care il are
            // astfel totalul de company este nCompany cerut
            if (cedatDinCompany < (int)potCedaCompany.size()) {
                plans[potCedaCompany[cedatDinCompany]].hasCompany = false;
                cedatDinCompany++;
            } 
        }
    }

    // setam op eq/ neq pt company
    std::vector<int> cuCompany;
    for (int i = 0; i < N; i++) {
        if (plans[i].hasCompany)
            cuCompany.push_back(i);
    }

    std::shuffle(cuCompany.begin(), cuCompany.end(), rng);

    int nEq = (int)(cuCompany.size() * config.companyEqMinPct / 100);
    for (int i = 0; i < (int)cuCompany.size(); i++) {
        plans[cuCompany[i]].companyOp =
            (i < nEq) ? OperatorType::EQ : OperatorType::NEQ;
    }

    return plans;
}

// Construim o subscriptie din plan
static Subscription buildSubscription(const Plan& plan, const Config& config, std::mt19937& rng) {
    Subscription sub;

    if (plan.hasCompany) {
        SubscriptionField f;
        f.fieldType = FieldType::COMPANY;
        f.op = plan.companyOp;
        f.stringValue = config.companies[rng() % config.companies.size()];
        sub.fields.push_back(f);
    }

    if (plan.hasValue) {
        SubscriptionField f;
        f.fieldType = FieldType::VALUE;
        f.op = OperatorType::GE;
        f.numericValue = config.subValueMin +
            (double)(rng() % 1000) / 1000 * (config.subValueMax - config.subValueMin);
        sub.fields.push_back(f);
    }

    if (plan.hasDrop) {
        SubscriptionField f;
        f.fieldType = FieldType::DROP;
        f.op = OperatorType::LE;
        f.numericValue = config.subDropMin +
            (double)(rng() % 1000) / 1000 * (config.subDropMax - config.subDropMin);
        sub.fields.push_back(f);
    }

    if (plan.hasVariation) {
        SubscriptionField f;
        f.fieldType = FieldType::VARIATION;
        f.op = OperatorType::LT;
        f.numericValue = config.subVariationMin +
            (double)(rng() % 1000) / 1000 * (config.subVariationMax - config.subVariationMin);
        sub.fields.push_back(f);
    }

    if (plan.hasDate) {
        SubscriptionField f;
        f.fieldType = FieldType::DATE;
        f.op = OperatorType::EQ;
        f.stringValue = config.dates[rng() % config.dates.size()];
        sub.fields.push_back(f);
    }

    return sub;
}

// Varianta secventiala, folosita pt un sg thread
std::vector<Subscription> generateSubscriptionsBalanced(const Config& config) {
    int N = (int)config.numSubscriptions;

    std::vector<Plan> plans = buildPlan(config);
    std::vector<Subscription> subs;
    subs.reserve(N);

    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < N; i++) {
        subs.push_back(buildSubscription(plans[i], config, rng));
    }
    printStats(subs);
    return subs;
}

// Varianta paralela
std::vector<Subscription> generateSubscriptionsBalancedParallel(const Config& config, size_t numThreads) {
    int N = (int)config.numSubscriptions;

    if (numThreads <= 1 || N == 0) {
        return generateSubscriptionsBalanced(config);
    }

    std::vector<Plan> plans = buildPlan(config);
    std::vector<Subscription> subs(N);

    int chunk = N / (int)numThreads;
    std::vector<std::thread> threads;

    for (int t = 0; t < (int)numThreads; t++) {
        int start = t * chunk;
        int end = (t == (int)numThreads - 1) ? N : start + chunk;

        threads.emplace_back([&, start, end]() {
            std::mt19937 rng(std::random_device{}());

            for (int i = start; i < end; i++) {
                subs[i] = buildSubscription(plans[i], config, rng);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
    printStats(subs);
    return subs;
}


