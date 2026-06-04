#pragma once
#include <string>

enum class MessageType {
    PUBLICATION,
    SUBSCRIPTION,
    BROKER_FORWARD
};

struct NetworkMessage {
    MessageType type;

    // pentru simplitate folosim string
    std::string payload;
};