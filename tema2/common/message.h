
#pragma once
#include <string>

enum class MessageType {
    SUBSCRIPTION,
    PUBLICATION,
    NOTIFICATION,
    BROKER_HELLO   // identificare conexiuni inter-broker
};

struct NetworkMessage {
    MessageType type;
    std::string payload;
};