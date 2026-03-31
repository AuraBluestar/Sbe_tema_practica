#include "models.h"
#include <string>

void parseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

       
        if (arg == "--nrPubs"  && i + 1 < argc) {
            config.numPublications = std::stoul(argv[++i]);
        }
        else if (arg == "--nrSubs"&& i + 1 < argc) {
            config.numSubscriptions = std::stoul(argv[++i]);
        }

        //frecv campuri
        else if (arg == "--companyFreq" && i + 1 < argc) {
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

        // operatori
        else if (arg == "--companyEq" && i + 1 < argc) {
            config.companyEqMinPct = std::stod(argv[++i]);
        }

        // intervale pubs
        else if (arg == "--pubValueMin" && i + 1 < argc) {
            config.pubValueMin = std::stod(argv[++i]);
        }
        else if (arg == "--pubValueMax" && i + 1 < argc) {
            config.pubValueMax = std::stod(argv[++i]);
        }
        else if (arg == "--pubDropMin" && i + 1 < argc) {
            config.pubDropMin = std::stod(argv[++i]);
        }
        else if (arg == "--pubDropMax" && i + 1 < argc) {
            config.pubDropMax = std::stod(argv[++i]);
        }
        else if (arg == "--pubVariationMin" && i + 1 < argc) {
            config.pubVariationMin = std::stod(argv[++i]);
        }
        else if (arg == "--pubVariationMax" && i + 1 < argc) {
            config.pubVariationMax = std::stod(argv[++i]);
        }

        // intervale subs
        else if (arg == "--subValueMin" && i + 1 < argc) {
            config.subValueMin = std::stod(argv[++i]);
        }
        else if (arg == "--subValueMax" && i + 1 < argc) {
            config.subValueMax = std::stod(argv[++i]);
        }
        else if (arg == "--subDropMin" && i + 1 < argc) {
            config.subDropMin = std::stod(argv[++i]);
        }
        else if (arg == "--subDropMax" && i + 1 < argc) {
            config.subDropMax = std::stod(argv[++i]);
        }
        else if (arg == "--subVariationMin" && i + 1 < argc) {
            config.subVariationMin = std::stod(argv[++i]);
        }
        else if (arg == "--subVariationMax" && i + 1 < argc) {
            config.subVariationMax = std::stod(argv[++i]);
        }

        
    }
}