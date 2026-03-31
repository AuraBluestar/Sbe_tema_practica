#include "models.h"



void parseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--companyFreq" && i + 1 < argc) {
            config.companyFreqPct = std::stod(argv[++i]);
        }
        else if (arg == "--valueFreq" && i + 1 < argc) {
            config.valueFreqPct = std::stod(argv[++i]);
        }
        else if (arg == "--dropFreq" && i + 1 < argc) {
            config.dropFreqPct = std::stod(argv[++i]);
        }
        else if (arg == "--variationFreq" && i + 1 < argc) {
            config.variationFreqPct = std::stod(argv[++i]);
        }
        else if (arg == "--dateFreq" && i + 1 < argc) {
            config.dateFreqPct = std::stod(argv[++i]);
        }
        else if (arg == "--eqCompany" && i + 1 < argc) {
            config.companyEqMinPct = std::stod(argv[++i]);
        }
        
        else if (arg == "--nrPubs" && i + 1 < argc) {
            config.numPublications = std::stoul(argv[++i]);
        }
        else if (arg == "--nrSubs" && i + 1 < argc) {
            config.numSubscriptions = std::stoul(argv[++i]);
        }
        // else if (arg == "--threads" && i + 1 < argc) {
        //     config.numThreads = std::stoul(argv[++i]);
        // }
    }
}