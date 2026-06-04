#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

#include "common/message.h"
#include "common/serialization.h"
#include "../src/publication_generator.h"
#include "../src/models.h"   // Publication din tema1

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    Config cfg;
    cfg.numPublications = 10; // momentan se trimit doar 10 pub generate

    auto pubs = generatePublicationsSequential(cfg);
    for(int i=0; i<cfg.numPublications; i++)
{
    Publication p=pubs[i];
    

    NetworkMessage msg;
    msg.type = MessageType::PUBLICATION;
    msg.payload = serializePublication(p);

    std::string data = serialize(msg);
    data += "\n";

    send(sock, data.c_str(), data.size(), 0);

    std::cout << "[PUBLISHER] sent\n";
    sleep(1);
}
}