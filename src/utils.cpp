#include "utils.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>


OperatorType randomStringOperator(std::mt19937& rng) {
    std::vector<OperatorType> ops = {
        OperatorType::EQ,
        OperatorType::NEQ
    };

    return ops[rng() % ops.size()];
}

OperatorType randomNumericOperator(std::mt19937& rng) {
    std::vector<OperatorType> ops = {
        OperatorType::EQ,
        OperatorType::NEQ,
        OperatorType::LT,
        OperatorType::LE,
        OperatorType::GT,
        OperatorType::GE
    };

    return ops[rng() % ops.size()];
}


double randomDouble(std::mt19937& rng, double minValue, double maxValue) {
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(rng);
}

int randomInt(std::mt19937& rng, int minValue, int maxValue) {
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng);
}

std::string randomChoice(std::mt19937& rng, const std::vector<std::string>& values) {
    if (values.empty()) {
        throw std::runtime_error("randomChoice: vectorul de valori este gol");
    }

    int index = randomInt(rng, 0, static_cast<int>(values.size()) - 1);
    return values[index];
}

std::string operatorToString(OperatorType op) {
    switch (op) {
        case OperatorType::EQ:  return "=";
        case OperatorType::NEQ: return "!=";
        case OperatorType::LT:  return "<";
        case OperatorType::LE:  return "<=";
        case OperatorType::GT:  return ">";
        case OperatorType::GE:  return ">=";
        default:                return "?";
    }
}

std::string fieldTypeToString(FieldType fieldType) {
    switch (fieldType) {
        case FieldType::COMPANY:   return "company";
        case FieldType::VALUE:     return "value";
        case FieldType::DROP:      return "drop";
        case FieldType::VARIATION: return "variation";
        case FieldType::DATE:      return "date";
        default:                   return "unknown";
    }
}

std::string formatDouble(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string publicationToString(const Publication& pub) {
    std::ostringstream out;

    out << "{";
    out << "(company,\"" << pub.company << "\");";
    out << "(value," << formatDouble(pub.value) << ");";
    out << "(drop," << formatDouble(pub.drop) << ");";
    out << "(variation," << formatDouble(pub.variation) << ");";
    out << "(date," << pub.date << ")";
    out << "}";

    return out.str();
}

std::string subscriptionToString(const Subscription& sub) {
    std::ostringstream out;
    out << "{";

    for (size_t i = 0; i < sub.fields.size(); i++) {
        const SubscriptionField& field = sub.fields[i];

        out << "(";
        out << fieldTypeToString(field.fieldType) << ",";
        out << operatorToString(field.op) << ",";

        if (field.fieldType == FieldType::COMPANY || field.fieldType == FieldType::DATE) {
            if (field.fieldType == FieldType::COMPANY) {
                out << "\"" << field.stringValue << "\"";
            } else {
                out << field.stringValue;
            }
        } else {
            out << formatDouble(field.numericValue);
        }

        out << ")";

        if (i + 1 < sub.fields.size()) {
            out << ";";
        }
    }

    out << "}";
    return out.str();
}