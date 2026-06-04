#pragma once

#include <string>

#include "../../src/models.h"
#include "message.h"

// =====================
// Network message
// =====================
std::string serialize(const NetworkMessage& msg);
NetworkMessage deserialize(const std::string& data);

// =====================
// Publication
// =====================
std::string serializePublication(const Publication& pub);
Publication deserializePublication(const std::string& data);

// =====================
// Subscription
// =====================
std::string serializeSubscription(const Subscription& sub);
Subscription deserializeSubscription(const std::string& data);