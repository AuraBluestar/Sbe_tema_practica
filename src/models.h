#pragma once

#include <string>
#include <vector>

// =====================
// Tipuri de campuri
// =====================
enum class FieldType {
    COMPANY,
    VALUE,
    DROP,
    VARIATION,
    DATE
};


enum class OperatorType {
    EQ,   
    NEQ,  
    LT,  
    LE,   
    GT,   
    GE    
};

// Publication (schema fixa)
struct Publication {
    std::string company;
    double value;
    double drop;
    double variation;
    std::string date;
};

// Camp din subscription
struct SubscriptionField {
    FieldType fieldType;
    OperatorType op;

    // simplificam: tinem ambele, folosim doar ce trebuie
    std::string stringValue;
    double numericValue = 0.0;
};

// Subscription (schema variabila)
struct Subscription {
    std::vector<SubscriptionField> fields;
};

// Configuratie generala
struct Config {
    // volum date
    size_t numPublications = 100;
    size_t numSubscriptions = 1000;
    size_t numThreads = 1;

    // frecventa campuri (subscriptii)
    double companyFreqPct = 90.0;
    double valueFreqPct = 70.0;
    double dropFreqPct = 50.0;
    double variationFreqPct = 60.0;
    double dateFreqPct = 40.0;

    // minim procent "=" pentru company
    double companyEqMinPct = 70.0;

    // intervale publication
    double pubValueMin = 0.0;
    double pubValueMax = 100.0;

    double pubDropMin = 0.0;
    double pubDropMax = 100.0;

    double pubVariationMin = -1.0;
    double pubVariationMax = 1.0;

    // intervale subscription (separate, cum a zis profesorul)
    double subValueMin = 0.0;
    double subValueMax = 100.0;

    double subDropMin = 0.0;
    double subDropMax = 100.0;

    double subVariationMin = -1.0;
    double subVariationMax = 1.0;

    // valori discrete
    std::vector<std::string> companies = {
        "Google", "Amazon", "Microsoft", "Apple", "Meta",
        "Netflix", "Tesla", "IBM", "Oracle", "NVIDIA"
    };

    std::vector<std::string> dates = {
        "01.01.2022", "15.01.2022", "02.02.2022", "10.03.2022",
        "25.04.2022", "13.05.2022", "30.06.2022",
        "14.07.2022", "09.08.2022", "21.09.2022"
    };
};