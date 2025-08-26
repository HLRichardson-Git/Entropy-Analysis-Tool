
#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <filesystem>

#include "models.h"
#include "../core/config.h"

namespace fs = std::filesystem;

class DataManager {
private:    
    std::string current_project_file;

public:
    DataManager() = default;
    ~DataManager() = default;
    
    // Initialization
    bool Initialize();
    Config::AppConfig loadAppConfig(const std::string& filePath);
    void saveAppConfig(const std::string& filePath, const Config::AppConfig& config);
    std::vector<std::string> GetVendorList();
    
    // Project management
    Project LoadProject(const std::string& relativePath);
    void DataManager::SaveProject(const Project& project, Config::AppConfig& appConfig);
    fs::path NewProject(const std::string& vendor, const std::string& repo, const std::string& projectName);
};