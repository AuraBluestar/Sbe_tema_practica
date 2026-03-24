#include "writer.h"
#include "utils.h"

#include <fstream>
#include <iostream>

void writePublicationsToFile(const std::string& filename, const std::vector<Publication>& publications) {
    std::ofstream fout(filename);

    if (!fout.is_open()) {
        std::cerr << "Eroare: nu s-a putut deschide fisierul " << filename << "\n";
        return;
    }

    for (const auto& pub : publications) {
        fout << publicationToString(pub) << "\n";
    }

    fout.close();
}

void writeSubscriptionsToFile(const std::string& filename, const std::vector<Subscription>& subscriptions) {
    std::ofstream fout(filename);

    if (!fout.is_open()) {
        std::cerr << "Eroare: nu s-a putut deschide fisierul " << filename << "\n";
        return;
    }

    for (const auto& sub : subscriptions) {
        fout << subscriptionToString(sub) << "\n";
    }

    fout.close();
}