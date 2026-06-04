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

    while (true) {
        NetworkMessage msg;
        msg.type = MessageType::PUBLICATION;
        msg.payload = "temp=25;city=iasi";

        std::string data = serialize(msg);

        send(sock, data.c_str(), data.size(), 0);

        sleep(1);
    }
}