
#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <filesystem>

#include "models.h"
#include "../core/config.h"
#include "histogram/histogram.h"

namespace fs = std::filesystem;

class DataManager {
private:    
    std::string current_project_file;

    // Helpers
    std::vector<std::string> GetVendorList();
    void UpdateOEsForProject(Project& project);

public:
    DataManager() = default;
    ~DataManager() = default;
    
    // Initialization
    bool Initialize(Config::AppConfig* config, Project* currentProjects);

    // Config
    Config::AppConfig loadAppConfig(const std::string& filePath);
    void saveAppConfig(const std::string& filePath, const Config::AppConfig& config);
    
    // Project management
    fs::path NewProject(const std::string& vendor, const std::string& repo, const std::string& projectName);
    Project LoadProject(const std::string& relativePath);
    void SaveProject(Project& project, Config::AppConfig& appConfig);
    void AddOEToProject(Project& project, const std::string& oeName, Config::AppConfig& appConfig);
    void DeleteOE(Project& project, int oeIndex, Config::AppConfig& appConfig);

    // Heuristic
    PrecomputedHistogram processHistogramForProject(Project& project, int oeIndex);
};