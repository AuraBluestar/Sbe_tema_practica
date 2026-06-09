#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common/message.h"
#include "common/serialization.h"
#include "../src/publication_generator.h"
#include "../src/models.h"

// ============================================================
//  Lansare: ./publisher <publisher_id> [num_publications] [delay_ms]
//  publisher_id: 1 sau 2
//  Se conecteaza ALEATORIU la unul dintre cei 3 brokeri
//  Emite publicatii la interval de delay_ms milisecunde
// ============================================================

int getPort(int brokerIdx) {
    return 5001 + brokerIdx; // broker 1=5001, 2=5002, 3=5003
}

int main(int argc, char* argv[]) {
   int delayMs = 100; // default 100ms intre publicatii
    int  publisherId = std::stoi(argv[1]);
    int numPubs = std::stoi(argv[2]);
    int numBrokers = std::stoi(argv[3]);

    // Alegem aleatoriu un broker
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> brokerDist(0, numBrokers - 1);
    int brokerIdx = brokerDist(rng);

    // Conectare
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(getPort(brokerIdx));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[PUB " << publisherId << "] Nu pot conecta la broker " 
                  << brokerIdx+1 << "\n";
        return 1;
    }

    std::cout << "[PUB " << publisherId << "] Conectat la broker " << brokerIdx+1 <<std::endl;

    // --------------------------------------------------------
    // Generare si emitere publicatii
    // --------------------------------------------------------
    
    int sent =0;
        Config cfg;
        cfg.numPublications = 1;


    for(int i=0;i<numPubs;i++){
    auto pubs = generatePublicationsSequential(cfg);

            Publication& p = pubs[0];

            NetworkMessage msg;
            msg.type = MessageType::PUBLICATION;
            msg.payload = serializePublication(p);

            std::string data = serialize(msg) + "\n";
            int ret = send(sock, data.c_str(), data.size(), 0);
            if (ret <= 0) {
                std::cerr << "[PUB " << publisherId << "] Eroare la trimitere, oprire\n";
                close(sock);
                return 1;
            }

            sent++;
            if (delayMs > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        
    }

    std::cout << "[PUB " << publisherId << "] Total trimise: " << sent << "\n";
    close(sock);
    return 0;
}