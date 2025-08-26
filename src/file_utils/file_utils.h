
#pragma once

#include "../core/config.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void from_json(const json& j, Project& p);
void to_json(json& j, const Project& p);

namespace Config {
    void from_json(const json& j, Config::AppConfig& c);
    void to_json(json& j, const Config::AppConfig& c);
}