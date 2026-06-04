#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

#include "common/message.h"
#include "common/serialization.h"
#include "../src/utils.h"
#include "../src/subscription_generator.h"
#include "../src/models.h"

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    // ======================
    // generare subscription simpla
    // ======================
    Config cfg;
    cfg.numSubscriptions = 1; // momentan se trimite doar 1 subs generata

    auto subs = generateSubscriptionsBalanced(cfg);

    Subscription sub = subs[0];
    for(int i=0; i<sub.fields.size(); i++)
    {
        std::cout<<"Field "<<i<<": type="<<fieldTypeToString(sub.fields[i].fieldType)
                 <<", op="<<operatorToString(sub.fields[i].op)
                 <<", strVal="<<sub.fields[i].stringValue
                 <<", numVal="<<sub.fields[i].numericValue<<"\n";
    }
    NetworkMessage msg;
    msg.type = MessageType::SUBSCRIPTION;
    msg.payload = serializeSubscription(sub);

    std::string data = serialize(msg);
    data += "\n";
    send(sock, data.c_str(), data.size(), 0);

    std::cout << "[SUBSCRIBER] subscription sent\n";

        
    std::string buffer;
    char temp[1024];

    while (true)
    {
        int bytes = recv(sock, temp, sizeof(temp), 0);
        if (bytes <= 0) break;

        buffer.append(temp, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string msgStr = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            NetworkMessage msg = deserialize(msgStr);

            if (msg.type == MessageType::NOTIFICATION)
            {
                Publication p = deserializePublication(msg.payload);

                std::cout << "[NOTIF] "
                        << p.company << " "
                        << p.value << " "
                        << p.date << "\n";
            }
        }
    }
}