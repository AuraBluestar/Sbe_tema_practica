#pragma once

#include <string>
#include <vector>
#include "models.h"

void writePublicationsToFile(const std::string& filename, const std::vector<Publication>& publications);
void writeSubscriptionsToFile(const std::string& filename, const std::vector<Subscription>& subscriptions);