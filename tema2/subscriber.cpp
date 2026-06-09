#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common/message.h"
#include "common/serialization.h"
#include "../src/utils.h"
#include "../src/subscription_generator.h"
#include "../src/models.h"

// ============================================================
//  Lansare: ./subscriber <subscriber_id> <num_subscriptions>
//  subscriber_id: 1, 2 sau 3
//  Subscriptiile sunt distribuite ROUND-ROBIN pe toti cei 3 brokeri
// ============================================================

std::mutex printMtx;
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
                long long latency = (emitTime > 0) ? (recvTime - emitTime) : 0;

                notificationsReceived++;
                if (latency > 0) totalLatencyUs += latency;

                std::lock_guard<std::mutex> lock(printMtx);
                std::cout << "[SUB " << subscriberId << " via B" << brokerIdx+1 << "] "
                          << "NOTIF: " << p.company << " val=" << p.value
                          << " latenta=" << latency << "us\n";
            }
        }
    }
}

// ============================================================
//  Main
// ============================================================
int main(int argc, char* argv[]) {
    int subscriberId = std::stoi(argv[1]);
int numSubs = std::stoi(argv[2]);
int numBrokers = std::stoi(argv[3]);

    std::cout << "[SUB " << subscriberId << "] Pornit, trimit " << numSubs 
              << " subscriptii distribuite pe " << numBrokers<< " brokeri\n";

    
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
    auto subs = generateSubscriptionsBalanced(cfg);

    // --------------------------------------------------------
    // Distribuire ROUND-ROBIN pe brokeri
    // Subscriptiile 0,3,6,... -> broker 0
    // Subscriptiile 1,4,7,... -> broker 1
    // Subscriptiile 2,5,8,... -> broker 2
    // --------------------------------------------------------
    for (int i = 0; i < (int)subs.size(); i++) {
        int brokerIdx = i % numBrokers;
        int sock = sockets[brokerIdx];

        // Payload: "<subscriberId>|<serialized_sub>"
        NetworkMessage msg;
        msg.type = MessageType::SUBSCRIPTION;
        msg.payload = std::to_string(subscriberId) + "|" + serializeSubscription(subs[i]);

        std::string data = serialize(msg) + "\n";
        send(sock, data.c_str(), data.size(), 0);

        std::cout << "[SUB " << subscriberId << "] Subscriptie " << i 
                  << " trimisa la broker " << brokerIdx+1 << "\n";
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