#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unordered_set>
#include <random>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common/message.h"
#include "common/serialization.h"
#include "../src/utils.h"
#include "../src/subscription_generator.h"
#include "../src/models.h"

// ============================================================
//  Lansare: ./subscriber <subscriber_id> <num_subscriptions> <num_brokers> [company_eq_pct]
//  subscriber_id: 1, 2 sau 3
//  Subscriptiile sunt trimise catre brokeri de intrare alesi aleatoriu.
// ============================================================

std::mutex printMtx;
std::mutex dedupMtx;
std::unordered_set<long long> receivedPublications;
std::atomic<long long> notificationsReceived{0};
std::atomic<long long> totalLatencyUs{0};

int getPort(int id) {
    return 5000 + id;
}

// ============================================================
//  Conectare la un broker
// ============================================================
int connectToBroker(int brokerIndex) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(getPort(brokerIndex));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[SUBSCRIBER] Nu pot conecta la broker " << brokerIndex+1 << "\n";
        close(sock);
        return -1;
    }
    return sock;
}

// ============================================================
//  Thread de receptie notificari de pe un socket
// ============================================================
void receiveNotifications(int sock, int subscriberId, int brokerIdx) {
    char temp[8192];
    std::string buffer;

    while (true) {
        int bytes = recv(sock, temp, sizeof(temp), 0);
        if (bytes <= 0) break;
        buffer.append(temp, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string msgStr = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (msgStr.empty()) continue;

            NetworkMessage msg = deserialize(msgStr);

            if (msg.type == MessageType::NOTIFICATION) {
                // Payload format: "<serialized_pub>|<emit_timestamp_us>"
                long long recvTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                std::string pubPayload = msg.payload;
                long long emitTime = 0;
                size_t sep = msg.payload.rfind('|');
                if (sep != std::string::npos) {
                    try {
                        emitTime = std::stoll(msg.payload.substr(sep + 1));
                        pubPayload = msg.payload.substr(0, sep);
                    } catch (...) {}
                }

                Publication p = deserializePublication(pubPayload);
                {
                    std::lock_guard<std::mutex> lock(dedupMtx);
                    if (receivedPublications.count(p.id)) {
                        continue;
                    }
                    receivedPublications.insert(p.id);
                }

                long long latency = (emitTime > 0) ? (recvTime - emitTime) : 0;

                notificationsReceived++;
                if (latency > 0) totalLatencyUs += latency;

                std::lock_guard<std::mutex> lock(printMtx);
                std::cout << "[SUB " << subscriberId << " via B" << brokerIdx+1 << "] "
                          << "NOTIF: id=" << p.id << " " << p.company << " val=" << p.value
                          << " latenta=" << latency << "us\n";
            }
        }
    }
}

// ============================================================
//  Main
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./subscriber <subscriber_id> <num_subscriptions> <num_brokers> [company_eq_pct]\n";
        return 1;
    }

    int subscriberId = std::stoi(argv[1]);
    int numSubs = std::stoi(argv[2]);
    int numBrokers = std::stoi(argv[3]);
    double companyEqPct = 70.0;
    if (argc >= 5) {
        companyEqPct = std::stod(argv[4]);
    }

    std::cout << "[SUB " << subscriberId << "] Pornit, trimit " << numSubs 
              << " subscriptii distribuite pe " << numBrokers
              << " brokeri, company EQ=" << companyEqPct << "%\n";

    
    std::vector<int> sockets;
    for (int i = 0; i < numBrokers; i++) {
        int sock = connectToBroker(i+1);
        if (sock < 0) {
            std::cerr << "[SUB " << subscriberId << "] EROARE: nu pot conecta la broker " << i+1 << "\n";
            return 1;
        }
        sockets.push_back(sock);
        std::cout << "[SUB " << subscriberId << "] Conectat la broker " << i+1 << "\n";
    }

    // --------------------------------------------------------
    // Generam subscriptii
    // --------------------------------------------------------
    Config cfg;
    cfg.numSubscriptions = numSubs;
    cfg.companyEqMinPct = companyEqPct;
    auto subs = generateSubscriptionsBalanced(cfg);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> brokerDist(0, numBrokers - 1);

    for (int i = 0; i < (int)subs.size(); i++) {
        NetworkMessage msg;
        msg.type = MessageType::SUBSCRIPTION;
        msg.payload = std::to_string(subscriberId) + "|" + serializeSubscription(subs[i]);

        int brokerIdx = brokerDist(rng);
        int sock = sockets[brokerIdx];
        std::string data = serialize(msg) + "\n";
        send(sock, data.c_str(), data.size(), 0);

        std::cout << "[SUB " << subscriberId << "] Subscriptie " << i
                  << " trimisa la broker de intrare random " << brokerIdx + 1 << "\n";
    }

    std::cout << "[SUB " << subscriberId << "] Toate subscriptiile trimise, astept notificari...\n";

    // --------------------------------------------------------
    // Thread de receptie per broker
    // --------------------------------------------------------
    std::vector<std::thread> recvThreads;
    for (int i = 0; i < (int)sockets.size(); i++) {
        recvThreads.emplace_back(receiveNotifications, sockets[i], subscriberId, i);
    }

    for (auto& t : recvThreads) t.join();

    // La final afisam statistici locale
    long long total = notificationsReceived.load();
    long long totalLat = totalLatencyUs.load();
    std::cout << "[SUB " << subscriberId << "] Total notificari primite: " << total << "\n";
    if (total > 0)
        std::cout << "[SUB " << subscriberId << "] Latenta medie: " 
                  << (totalLat / total) << " us\n";

    for (int s : sockets) close(s);
    return 0;
}
