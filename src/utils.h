#pragma once

#include <random>
#include <string>
#include <vector>
#include "models.h"

double randomDouble(std::mt19937& rng, double minValue, double maxValue);
int randomInt(std::mt19937& rng, int minValue, int maxValue);
OperatorType randomNumericOperator(std::mt19937& rng);
OperatorType randomStringOperator(std::mt19937& rng);

std::string randomChoice(std::mt19937& rng, const std::vector<std::string>& values);

std::string operatorToString(OperatorType op);
std::string fieldTypeToString(FieldType fieldType);

std::string formatDouble(double value);

std::string publicationToString(const Publication& pub);
std::string subscriptionToString(const Subscription& sub);