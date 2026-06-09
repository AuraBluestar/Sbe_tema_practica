#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <chrono>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common/message.h"
#include "common/serialization.h"
#include "common/matcher.h"
#include "../src/models.h"

std::mutex mtx;

// ---------------------------
// subscriptions local
// ---------------------------
struct SubscriberInfo {
    int socket;
    int subscriberId;
    Subscription subscription;
};

std::vector<SubscriberInfo> localSubscribers;

// ---------------------------
// peer broker sockets
// ---------------------------
std::unordered_map<int, int> brokerSockets;

// ---------------------------
// company ownership (MANY brokers per company)
// ---------------------------
std::unordered_map<std::string, std::unordered_set<int>> companyMap;

// ---------------------------
int myBrokerId;
int numBrokers;


struct BrokerConfig {
    int id;
    int port;
    std::vector<std::pair<std::string, int>> peers; 
};
// ============================================================
// INIT COMPANY OWNERSHIP (IMPORTANT PART)
// ============================================================
void initCompanyMap() {

    std::vector<std::string> companies = {
        "Google", "Amazon", "Microsoft", "Apple", "Meta",
        "Netflix", "Tesla", "IBM", "Oracle", "NVIDIA"
    };

    // deterministic but MULTI-OWNER distribution
    for (size_t i = 0; i < companies.size(); i++) {

        const std::string& c = companies[i];

        // fiecare companie merge la 1-2 brokeri (overlap)
        int b1 = (i % numBrokers) + 1;
        int b2 = ((i + 1) % numBrokers) + 1;

        companyMap[c].insert(b1);
        companyMap[c].insert(b2);
    }
    for(int i=0;i<companies.size();i++){
        std::cout << "Company " << companies[i] << " -> brokers: ";
        for (int b : companyMap[companies[i]]) {
            std::cout << b << " ";
        }
        std::cout << "\n";
    }
}

// ============================================================
void sendMsg(int sock, const NetworkMessage& msg) {
    std::string data = serialize(msg) + "\n";
    send(sock, data.c_str(), data.size(), 0);
}

// ============================================================
void matchAndNotify(const Publication& pub) {

    std::lock_guard<std::mutex> lock(mtx);

    for (auto& sub : localSubscribers) {
        if (matches(pub, sub.subscription)) {

            NetworkMessage n;
            n.type = MessageType::NOTIFICATION;

            auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();

            n.payload = serializePublication(pub) + "|" + std::to_string(now);

            sendMsg(sub.socket, n);
        }
    }
}

// ============================================================
void handleClient(int clientSock) {

    char buf[8192];
    std::string buffer;
    bool isBrokerConn = false;

    while (true) {
        int bytes = recv(clientSock, buf, sizeof(buf), 0);
        if (bytes <= 0) break;

        buffer.append(buf, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {

            std::string msgStr = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            NetworkMessage msg = deserialize(msgStr);

            // -------------------------
            // broker handshake
            // -------------------------
            if (msg.type == MessageType::BROKER_HELLO) {
            isBrokerConn = true;

            int peerId = std::stoi(msg.payload);

            std::lock_guard<std::mutex> lock(mtx);
            brokerSockets[peerId] = clientSock;
        }

            // -------------------------
            // subscription
            // -------------------------
            else if (msg.type == MessageType::SUBSCRIPTION) {

                Subscription sub;
                int subId = 0;

                size_t sep = msg.payload.find('|');

                if (sep != std::string::npos) {
                    subId = std::stoi(msg.payload.substr(0, sep));
                    sub = deserializeSubscription(msg.payload.substr(sep + 1));
                }
                std::cout<<"[BROKER " << myBrokerId << "] Received SUBSCRIPTION from SUB " << subId 
                          << ", company=" << sub.fields[0].stringValue << "\n";
                std::lock_guard<std::mutex> lock(mtx);
                localSubscribers.push_back({clientSock, subId, sub});
            }

            // -------------------------
            // PUBLICATION ROUTING (MULTI-OWNER)
            // -------------------------
            else if (msg.type == MessageType::PUBLICATION) {

                Publication pub = deserializePublication(msg.payload);
                std::cout<<"[BROKER " << myBrokerId << "] Received PUBLICATION: " << pub.company 
                          << " val=" << pub.value << "\n";
                auto it = companyMap.find(pub.company);

                if (it == companyMap.end()) {
                    // fallback: local only
                    matchAndNotify(pub);
                    continue;
                }

                const auto& owners = it->second;

                // forward to ALL responsible brokers
                for (int owner : owners) {

                    if (owner == myBrokerId) {
                        matchAndNotify(pub);
                    } else {
                        if (brokerSockets.count(owner)) {
                            sendMsg(brokerSockets[owner], msg);
                        }
                    }
                }
            }
        }
    }

    if (!isBrokerConn) {
        std::lock_guard<std::mutex> lock(mtx);

        localSubscribers.erase(
            std::remove_if(localSubscribers.begin(), localSubscribers.end(),
                [&](const SubscriberInfo& s) {
                    return s.socket == clientSock;
                }),
            localSubscribers.end()
        );
    }

    close(clientSock);
}

// ============================================================
BrokerConfig getConfig(int id) {
    BrokerConfig cfg;
    cfg.id = id;

    if (id == 1) {
        cfg.port = 5001;
        cfg.peers = {{"127.0.0.1", 5002}, {"127.0.0.1", 5003}};
    } else if (id == 2) {
        cfg.port = 5002;
        cfg.peers = {{"127.0.0.1", 5001}, {"127.0.0.1", 5003}};
    } else {
        cfg.port = 5003;
        cfg.peers = {{"127.0.0.1", 5001}, {"127.0.0.1", 5002}};
    }

    return cfg;
}

// ============================================================
void connectToPeers(const BrokerConfig& cfg) {

    for (auto& [ip, port] : cfg.peers) {

        int sock = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        while (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        NetworkMessage hello;
        hello.type = MessageType::BROKER_HELLO;
        hello.payload = std::to_string(cfg.id);

        sendMsg(sock, hello);
    }
}

// ============================================================
int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: ./broker <id> <num_brokers>\n";
        return 1;
    }

    myBrokerId = std::stoi(argv[1]);
    numBrokers = std::stoi(argv[2]);

    BrokerConfig cfg = getConfig(myBrokerId);

    initCompanyMap();

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 20);

    std::cout << "[BROKER " << myBrokerId << "] started\n";

    std::thread([&cfg]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        connectToPeers(cfg);
    }).detach();

    while (true) {
        int c = accept(serverSock, nullptr, nullptr);
        if (c < 0) continue;

        std::thread(handleClient, c).detach();
    }
}