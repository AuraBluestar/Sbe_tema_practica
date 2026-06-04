#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>

#include "message.h"
#include "serialization.h"

std::vector<int> subscribers;
std::mutex mtx;

void handleClient(int clientSock) {
    char buffer[1024];

    while (true) {
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';

        NetworkMessage msg = deserialize(buffer);

        if (msg.type == MessageType::SUBSCRIPTION) {
            std::lock_guard<std::mutex> lock(mtx);
            subscribers.push_back(clientSock);
            std::cout << "New subscriber\n";
        }

        if (msg.type == MessageType::PUBLICATION) {
            std::lock_guard<std::mutex> lock(mtx);

            for (int subSock : subscribers) {
                send(subSock, buffer, bytes, 0);
            }
        }
    }

    close(clientSock);
}

int main() {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 10);

    std::cout << "Broker started...\n";

    while (true) {
        int clientSock = accept(serverSock, nullptr, nullptr);

        std::thread(handleClient, clientSock).detach();
    }
}