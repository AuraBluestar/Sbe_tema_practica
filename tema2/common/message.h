
#pragma once
#include <string>

enum class MessageType {
    SUBSCRIPTION,
    PUBLICATION,
    NOTIFICATION,
    BROKER_HELLO,          // identificare conexiuni inter-broker
    BROKER_SUBSCRIPTION,   // subscriptie rutata prin overlay
    ROUTED_NOTIFICATION    // notificare intoarsa la brokerul de intrare
};

struct NetworkMessage {
    MessageType type;
    std::string payload;
};
