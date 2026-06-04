#include "message.h"
#include <sstream>

inline std::string serialize(const NetworkMessage& msg) {
    //std::string s = std::to_string((int)msg.type) + "|" + msg.payload;
    std::ostringstream out;
    out << (int)msg.type << "|" << msg.payload;
    return out.str();
}

inline NetworkMessage deserialize(const std::string& data) {
    std::istringstream in(data);

    NetworkMessage msg;
    int typeInt;

    std::string payload;

    in >> typeInt;
    in.get(); // consuma '|'
    std::getline(in, payload);

    msg.type = (MessageType)typeInt;
    msg.payload = payload;

    return msg;
}