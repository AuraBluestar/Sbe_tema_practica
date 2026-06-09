#include "serialization.h"
#include <sstream>

// ---------------------
// Publication
// ---------------------
std::string serializePublication(const Publication& p)
{
    return std::to_string(p.id) + "|" +
           p.company + "|" +
           std::to_string(p.value) + "|" +
           std::to_string(p.drop) + "|" +
           std::to_string(p.variation) + "|" +
           p.date;
}

Publication deserializePublication(const std::string& data)
{
    Publication p;
    std::stringstream ss(data);

    std::string token;

    std::getline(ss, token, '|');
    p.id = std::stoll(token);

    std::getline(ss, p.company, '|');

    std::getline(ss, token, '|');
    p.value = std::stod(token);

    std::getline(ss, token, '|');
    p.drop = std::stod(token);

    std::getline(ss, token, '|');
    p.variation = std::stod(token);

    std::getline(ss, p.date, '|');

    return p;
}


std::string serializeSubscription(const Subscription& sub)
{
    std::string out;

    for (size_t i = 0; i < sub.fields.size(); i++)
    {
        const auto& f = sub.fields[i];

        out += std::to_string((int)f.fieldType) + "," +
               std::to_string((int)f.op) + "," +
               f.stringValue + "," +
               std::to_string(f.numericValue);

        if (i + 1 < sub.fields.size())
            out += ";";
    }

    return out;
}


Subscription deserializeSubscription(const std::string& data)
{
    Subscription sub;

    std::stringstream ss(data);
    std::string fieldBlock;

    while (std::getline(ss, fieldBlock, ';'))
    {
        std::stringstream fs(fieldBlock);

        SubscriptionField f;
        std::string token;

        std::getline(fs, token, ',');
        f.fieldType = (FieldType)std::stoi(token);

        std::getline(fs, token, ',');
        f.op = (OperatorType)std::stoi(token);

        std::getline(fs, f.stringValue, ',');

        std::getline(fs, token, ',');
        f.numericValue = std::stod(token);

        sub.fields.push_back(f);
    }

    return sub;
}

NetworkMessage deserialize(const std::string& data)
{
    NetworkMessage msg;
    std::stringstream ss(data);

    std::string token;

    std::getline(ss, token, '#');
    msg.type = (MessageType)std::stoi(token);

    std::getline(ss, msg.payload); // TOT RESTUL intact

    return msg;
}

std::string serialize(const NetworkMessage& msg)
{
    return std::to_string((int)msg.type) + "#" + msg.payload;
}
