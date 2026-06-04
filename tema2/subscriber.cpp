#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

#include "message.h"
#include "serialization.h"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    // trimite subscription
    NetworkMessage sub;
    sub.type = MessageType::SUBSCRIPTION;
    sub.payload = "city=iasi";

    std::string data = serialize(sub);
    send(sock, data.c_str(), data.size(), 0);

    char buffer[1024];

    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';

        auto msg = deserialize(buffer);

        std::cout << "Received: " << msg.payload << "\n";
    }
}