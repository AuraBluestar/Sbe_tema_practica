#pragma once
#include <string>

enum class MessageType
{
    PUBLICATION,
    SUBSCRIPTION,
    BROKER_FORWARD,
    NOTIFICATION
};

struct NetworkMessage
{
    MessageType type;
    std::string payload;
};