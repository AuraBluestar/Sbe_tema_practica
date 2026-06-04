#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unistd.h>
#include <arpa/inet.h>

#include "common/message.h"
#include "common/serialization.h"
#include "common/matcher.h"
#include "../src/utils.h"
#include "../src/models.h"

std::mutex mtx;


struct SubscriberInfo
{
    int socket;
    Subscription subscription;
};

std::vector<SubscriberInfo> subscribers;
void handleClient(int clientSock)
{
    char temp[4096];
    std::string buffer;

    while (true)
    {
        int bytes = recv(clientSock, temp, sizeof(temp), 0);

        if (bytes <= 0)
            break;

        buffer.append(temp, bytes);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string msgStr = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            NetworkMessage msg = deserialize(msgStr);

            std::lock_guard<std::mutex> lock(mtx);

            // =====================
            // SUBSCRIPTION
            // =====================
            if (msg.type == MessageType::SUBSCRIPTION)
            {
                Subscription sub = deserializeSubscription(msg.payload);
                std::cout<<"Subscription received "<< sub.fields.size() << " fields\n";
                for(int i=0; i<sub.fields.size(); i++)
                {
                    std::cout<<"Field "<<i<<": type="<<fieldTypeToString(sub.fields[i].fieldType)
                             <<", op="<<operatorToString(sub.fields[i].op)
                             <<", strVal="<<sub.fields[i].stringValue
                             <<", numVal="<<sub.fields[i].numericValue<<"\n";
                }
                subscribers.push_back({clientSock, sub});
            }

            // =====================
            // PUBLICATION
            // =====================
            else if (msg.type == MessageType::PUBLICATION)
            {
               Publication pub=deserializePublication(msg.payload);
                std::cout<<"Publication received: "<<pub.company<<" "<<pub.value<<" "<<pub.date<<" "<<pub.drop<< " "<<pub.variation<<"\n";
    
                 for (const auto& subInfo : subscribers)
                 {
                      if (matches(pub, subInfo.subscription))
                      {
                            NetworkMessage notif;
                            notif.type = MessageType::NOTIFICATION;
                            notif.payload = serializePublication(pub) + "\n";
    
                            send(subInfo.socket, serialize(notif).c_str(), serialize(notif).size(), 0);
                      }
                 }
            }
        }
    }

    close(clientSock);
}

int main()
{
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 10);

    std::cout << "[BROKER] Started on port 5000\n";

    while (true)
    {
        int clientSock = accept(serverSock, nullptr, nullptr);

        std::thread(handleClient, clientSock).detach();
    }
}