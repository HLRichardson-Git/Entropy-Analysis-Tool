
#pragma once

#include "../core/config.h"

#include <ImGuiFileDialog.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

void from_json(const json& j, Project& p);
void to_json(json& j, const Project& p);

namespace Config {
    void from_json(const json& j, Config::AppConfig& c);
    void to_json(json& j, const Config::AppConfig& c);
}

std::optional<std::string> FileSelector(
    const std::string& dialogKey,       // Unique key for this dialog
    const std::string& buttonLabel,     // Label for the button that opens dialog
    const std::string& fileFilters = ".*", // e.g. ".txt,.bin"
    const std::string& initialPath = "."    // Start folder
);

std::optional<fs::path> CopyFileToDirectory(const fs::path& sourcePath, const fs::path& destDir);