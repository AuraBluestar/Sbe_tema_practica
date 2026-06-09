#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <chrono>
#include <sstream>

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
struct StoredSubscription {
    int socket;
    int subscriberId;
    int originBrokerId;
    Subscription subscription;
};

std::vector<StoredSubscription> storedSubscriptions;
std::unordered_map<int, int> localSubscriberSockets;

// ---------------------------
// peer broker sockets
// ---------------------------
std::unordered_map<int, int> brokerSockets;

// ---------------------------
// company ownership (MANY brokers per company)
// ---------------------------
std::unordered_map<std::string, std::unordered_set<int>> companyMap;

// ---------------------------
// deduplication
// ---------------------------
std::unordered_set<long long> processedPublications;
std::unordered_map<int, std::unordered_set<long long>> deliveredToSubscriber;
std::unordered_map<std::string, int> subscriptionRouteCursor;

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

int brokerIdFromPort(int port) {
    return port - 5000;
}

const SubscriptionField* findCompanyField(const Subscription& sub) {
    for (const auto& field : sub.fields) {
        if (field.fieldType == FieldType::COMPANY) {
            return &field;
        }
    }

    return nullptr;
}

std::vector<int> getCompanyOwners(const std::string& company) {
    std::vector<int> owners;
    auto it = companyMap.find(company);

    if (it == companyMap.end()) {
        return owners;
    }

    for (int owner : it->second) {
        owners.push_back(owner);
    }

    std::sort(owners.begin(), owners.end());
    return owners;
}

void storeSubscriptionLocked(int socket, int subscriberId, int originBrokerId, const Subscription& sub) {
    storedSubscriptions.push_back({socket, subscriberId, originBrokerId, sub});
}

void sendNotificationToLocalSubscriberLocked(int subscriberId, const Publication& pub, long long emitTime) {
    if (deliveredToSubscriber[subscriberId].count(pub.id)) {
        return;
    }

    auto it = localSubscriberSockets.find(subscriberId);
    if (it == localSubscriberSockets.end()) {
        return;
    }

    deliveredToSubscriber[subscriberId].insert(pub.id);

    NetworkMessage n;
    n.type = MessageType::NOTIFICATION;
    n.payload = serializePublication(pub) + "|" + std::to_string(emitTime);

    sendMsg(it->second, n);
}

void routeNotificationToOriginLocked(const StoredSubscription& sub, const Publication& pub, long long emitTime) {
    if (sub.originBrokerId == myBrokerId) {
        sendNotificationToLocalSubscriberLocked(sub.subscriberId, pub, emitTime);
        return;
    }

    if (deliveredToSubscriber[sub.subscriberId].count(pub.id)) {
        return;
    }

    deliveredToSubscriber[sub.subscriberId].insert(pub.id);

    if (!brokerSockets.count(sub.originBrokerId)) {
        std::cout << "[BROKER " << myBrokerId << "] Missing connection to origin B"
                  << sub.originBrokerId << " for NOTIFICATION id=" << pub.id << "\n";
        return;
    }

    NetworkMessage routed;
    routed.type = MessageType::ROUTED_NOTIFICATION;
    routed.payload = std::to_string(sub.subscriberId) + "|" +
                     serializePublication(pub) + "|" +
                     std::to_string(emitTime);

    sendMsg(brokerSockets[sub.originBrokerId], routed);
    std::cout << "[BROKER " << myBrokerId << "] Routed NOTIFICATION id=" << pub.id
              << " for SUB " << sub.subscriberId << " to origin B"
              << sub.originBrokerId << "\n";
}

void routeSubscriptionFromClient(int clientSock, int subscriberId, const Subscription& sub) {
    std::lock_guard<std::mutex> lock(mtx);
    localSubscriberSockets[subscriberId] = clientSock;

    const SubscriptionField* companyField = findCompanyField(sub);

    if (companyField != nullptr && companyField->op == OperatorType::EQ) {
        std::vector<int> owners = getCompanyOwners(companyField->stringValue);
        int targetBroker = myBrokerId;

        if (!owners.empty()) {
            int cursor = subscriptionRouteCursor[companyField->stringValue]++;
            targetBroker = owners[cursor % owners.size()];
        }

        if (targetBroker == myBrokerId) {
            storeSubscriptionLocked(clientSock, subscriberId, myBrokerId, sub);
            std::cout << "[BROKER " << myBrokerId << "] Stored local SUBSCRIPTION from SUB "
                      << subscriberId << " for " << companyField->stringValue << "\n";
        } else if (brokerSockets.count(targetBroker)) {
            NetworkMessage routed;
            routed.type = MessageType::BROKER_SUBSCRIPTION;
            routed.payload = std::to_string(myBrokerId) + "|" +
                             std::to_string(subscriberId) + "|" +
                             serializeSubscription(sub);

            sendMsg(brokerSockets[targetBroker], routed);
            std::cout << "[BROKER " << myBrokerId << "] Routed SUBSCRIPTION from SUB "
                      << subscriberId << " for " << companyField->stringValue
                      << " to B" << targetBroker << "\n";
        } else {
            storeSubscriptionLocked(clientSock, subscriberId, myBrokerId, sub);
            std::cout << "[BROKER " << myBrokerId << "] Stored SUBSCRIPTION locally because B"
                      << targetBroker << " is unavailable\n";
        }

        return;
    }

    storeSubscriptionLocked(clientSock, subscriberId, myBrokerId, sub);
    std::cout << "[BROKER " << myBrokerId << "] Stored complex SUBSCRIPTION from SUB "
              << subscriberId << " and replicated through overlay\n";

    for (const auto& [brokerId, sock] : brokerSockets) {
        NetworkMessage routed;
        routed.type = MessageType::BROKER_SUBSCRIPTION;
        routed.payload = std::to_string(myBrokerId) + "|" +
                         std::to_string(subscriberId) + "|" +
                         serializeSubscription(sub);

        sendMsg(sock, routed);
    }
}

// ============================================================
void matchAndNotifyLocked(const Publication& pub) {
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (auto& sub : storedSubscriptions) {
        if (matches(pub, sub.subscription)) {
            routeNotificationToOriginLocked(sub, pub, now);
        }
    }
}

// ============================================================
bool markPublicationProcessed(long long publicationId) {
    std::lock_guard<std::mutex> lock(mtx);

    if (processedPublications.count(publicationId)) {
        return false;
    }

    processedPublications.insert(publicationId);
    return true;
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
                std::cout << "[BROKER " << myBrokerId << "] Broker connection accepted from B"
                          << peerId << "\n";
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
                routeSubscriptionFromClient(clientSock, subId, sub);
            }

            else if (msg.type == MessageType::BROKER_SUBSCRIPTION) {
                size_t firstSep = msg.payload.find('|');
                size_t secondSep = msg.payload.find('|', firstSep + 1);

                if (firstSep == std::string::npos || secondSep == std::string::npos) {
                    continue;
                }

                int originBrokerId = std::stoi(msg.payload.substr(0, firstSep));
                int subscriberId = std::stoi(msg.payload.substr(firstSep + 1, secondSep - firstSep - 1));
                Subscription sub = deserializeSubscription(msg.payload.substr(secondSep + 1));

                std::lock_guard<std::mutex> lock(mtx);
                storeSubscriptionLocked(-1, subscriberId, originBrokerId, sub);

                const SubscriptionField* companyField = findCompanyField(sub);
                std::cout << "[BROKER " << myBrokerId << "] Stored routed SUBSCRIPTION for SUB "
                          << subscriberId << " from origin B" << originBrokerId;
                if (companyField != nullptr) {
                    std::cout << ", company=" << companyField->stringValue;
                }
                std::cout << "\n";
            }

            else if (msg.type == MessageType::ROUTED_NOTIFICATION) {
                size_t firstSep = msg.payload.find('|');
                size_t lastSep = msg.payload.rfind('|');

                if (firstSep == std::string::npos || lastSep == std::string::npos || firstSep == lastSep) {
                    continue;
                }

                int subscriberId = std::stoi(msg.payload.substr(0, firstSep));
                Publication pub = deserializePublication(msg.payload.substr(firstSep + 1, lastSep - firstSep - 1));
                long long emitTime = std::stoll(msg.payload.substr(lastSep + 1));

                std::lock_guard<std::mutex> lock(mtx);
                sendNotificationToLocalSubscriberLocked(subscriberId, pub, emitTime);
            }

            // -------------------------
            // PUBLICATION ROUTING (MULTI-OWNER)
            // -------------------------
            else if (msg.type == MessageType::PUBLICATION) {

                Publication pub = deserializePublication(msg.payload);
                if (!markPublicationProcessed(pub.id)) {
                    std::cout << "[BROKER " << myBrokerId << "] Duplicate PUBLICATION ignored: id="
                              << pub.id << "\n";
                    continue;
                }

                std::cout<<"[BROKER " << myBrokerId << "] Received PUBLICATION: id=" << pub.id
                          << " " << pub.company 
                          << " val=" << pub.value << "\n";
                auto it = companyMap.find(pub.company);

                if (it == companyMap.end()) {
                    // fallback: local only
                    std::lock_guard<std::mutex> lock(mtx);
                    matchAndNotifyLocked(pub);
                    continue;
                }

                const auto& owners = it->second;

                // forward to ALL responsible brokers
                std::lock_guard<std::mutex> lock(mtx);
                for (int owner : owners) {

                    if (owner == myBrokerId) {
                        matchAndNotifyLocked(pub);
                    } else {
                        if (brokerSockets.count(owner)) {
                            sendMsg(brokerSockets[owner], msg);
                            std::cout << "[BROKER " << myBrokerId << "] Forwarded PUBLICATION id="
                                      << pub.id << " to B" << owner << "\n";
                        } else {
                            std::cout << "[BROKER " << myBrokerId << "] Missing connection to B"
                                      << owner << " for PUBLICATION id=" << pub.id << "\n";
                        }
                    }
                }
            }
        }
    }

    if (!isBrokerConn) {
        std::lock_guard<std::mutex> lock(mtx);

        for (auto it = localSubscriberSockets.begin(); it != localSubscriberSockets.end(); ) {
            if (it->second == clientSock) {
                it = localSubscriberSockets.erase(it);
            } else {
                ++it;
            }
        }

        storedSubscriptions.erase(
            std::remove_if(storedSubscriptions.begin(), storedSubscriptions.end(),
                [&](const StoredSubscription& s) {
                    return s.socket == clientSock;
                }),
            storedSubscriptions.end()
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

        int peerId = brokerIdFromPort(port);

        {
            std::lock_guard<std::mutex> lock(mtx);
            brokerSockets[peerId] = sock;
        }

        NetworkMessage hello;
        hello.type = MessageType::BROKER_HELLO;
        hello.payload = std::to_string(cfg.id);

        sendMsg(sock, hello);
        std::cout << "[BROKER " << myBrokerId << "] Connected to B" << peerId << "\n";
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
