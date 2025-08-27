
#include "data_manager.h"
#include "../file_utils/file_utils.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

bool DataManager::Initialize(Config::AppConfig* config, Project* currentProject) {
    *config = loadAppConfig("../../data/app.json");
    if (!config->lastOpenedProject.path.empty()) {
        *currentProject = LoadProject(config->lastOpenedProject.path);
    }

    return true;
}

Config::AppConfig DataManager::loadAppConfig(const std::string& relativePath) {
    Config::AppConfig config;

    if (relativePath.empty()) {
        std::cerr << "No config file path provided." << std::endl;
        return config;
    }

    // Resolve relative path against current working directory
    fs::path fullPath = fs::current_path() / relativePath;

    if (!fs::exists(fullPath)) {
        std::cerr << "Config file does not exist: " << fullPath << std::endl;
        return config;
    }

    // Open and parse JSON
    std::ifstream in(fullPath);
    if (!in.is_open()) {
        std::cerr << "Failed to open config file: " << fullPath << std::endl;
        return config;
    }

    try {
        nlohmann::json j;
        in >> j;
        config = j.get<Config::AppConfig>();
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Failed to parse config JSON: " << e.what() << std::endl;
    }

    config.vendorsList = GetVendorList();

    return config;
}

void DataManager::saveAppConfig(const std::string& filePath, const Config::AppConfig& config) {
    // Read existing JSON
    std::ifstream in(filePath);
    nlohmann::json j;
    if (in.is_open()) {
        in >> j;
        in.close();
    }

    // Update only the app config fields
    j["lastOpenedProject"] = config.lastOpenedProject;
    j["savedProjects"] = config.savedProjects;

    // Write back
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Failed to open " << filePath << " for writing!" << std::endl;
        return;
    }
    out << j.dump(4);
}

fs::path DataManager::NewProject(const std::string& vendor, const std::string& repo, const std::string& projectName) {

    fs::path projectDir = fs::path("../../projects") / vendor / repo / projectName;
    fs::create_directories(projectDir);

    fs::path projectJsonPath = projectDir / "project.json";

    nlohmann::json projectTemplate;
    projectTemplate["name"] = projectName;
    projectTemplate["path"] = fs::relative(projectJsonPath, fs::current_path()).string();
    projectTemplate["currentTabIndex"] = 0;
    projectTemplate["hasJentHeuristic"] = true;
    projectTemplate["operationalEnvironments"] = {
        {{"name", "OE1"}, {"rawFilePath", "raw_data_oe1.bin"}, {"restartFilePath", "restart_data_oe1.bin"}, {"minEntropy", 0.0}, {"passed", false}},
        {{"name", "OE2"}, {"rawFilePath", "raw_data_oe2.bin"}, {"restartFilePath", "restart_data_oe2.bin"}, {"minEntropy", 0.0}, {"passed", false}}
    };

    std::ofstream out(projectJsonPath);
    out << projectTemplate.dump(4);

    return projectJsonPath;
}

Project DataManager::LoadProject(const std::string& filename) {
    Project proj;

    fs::path fullPath = fs::absolute(filename);
    if (!fs::exists(fullPath)) {
        std::cerr << "Project file does not exist: " << fullPath.string() << std::endl;
        return proj;
    }

    // Load JSON
    std::ifstream in(fullPath);
    if (!in) {
        std::cerr << "Failed to open project file: " << fullPath.string() << std::endl;
        return proj;
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse project JSON: " << e.what() << std::endl;
        return proj;
    }

    // Read name from JSON
    if (j.contains("name") && j["name"].is_string()) {
        proj.name = j["name"].get<std::string>();
    } else {
        proj.name = fullPath.stem().string(); // fallback
    }

    // Store the path (absolute internally)
    proj.path = fullPath.string();

    // TODO: load other fields as needed, e.g., operationalEnvironments, hasJentHeuristic, etc.

    return proj;
}

void DataManager::SaveProject(const Project& project, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;

    fs::path projectJsonPath = project.path;

    // Build JSON template similar to NewProject
    nlohmann::json projectJson;
    projectJson["name"] = project.name;
    projectJson["path"] = fs::relative(projectJsonPath, fs::current_path()).string();
    // We do not have the OE and other logic in, so use placeholder for now
    projectJson["currentTabIndex"] = 0;
    projectJson["hasJentHeuristic"] = true;
    projectJson["operationalEnvironments"] = {
        {{"name", "OE1"}, {"rawFilePath", "raw_data_oe1.bin"}, {"restartFilePath", "restart_data_oe1.bin"}, {"minEntropy", 0.0}, {"passed", false}},
        {{"name", "OE2"}, {"rawFilePath", "raw_data_oe2.bin"}, {"restartFilePath", "restart_data_oe2.bin"}, {"minEntropy", 0.0}, {"passed", false}}
    };

    // Write project.json
    std::ofstream out(projectJsonPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open project file for writing: " << projectJsonPath << std::endl;
        return;
    }
    out << projectJson.dump(4);
    out.close();

    // Update savedProjects in appConfig if not already present
    auto it = std::find_if(
        appConfig.savedProjects.begin(),
        appConfig.savedProjects.end(),
        [&](const Project& p) { return p.path == project.path; }
    );

    if (it == appConfig.savedProjects.end()) {
        appConfig.savedProjects.push_back(project);
    }

    // Persist app.json
    saveAppConfig("../../data/app.json", appConfig);
}

std::vector<std::string> DataManager::GetVendorList() {
    std::vector<std::string> vendors;
    std::ifstream in("../../data/vendorList.json");
    if (in) {
        nlohmann::json j;
        in >> j;
        for (auto& v : j) {
            vendors.push_back(v.get<std::string>());
        }
    }
    return vendors;
}