
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
    projectTemplate["operationalEnvironments"] = { {} };

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

    // Load operational environments
    proj.operationalEnvironments.clear();
    if (j.contains("operationalEnvironments") && j["operationalEnvironments"].is_array()) {
        for (auto& oeJson : j["operationalEnvironments"]) {
            // Skip null or non-object entries
            if (!oeJson.is_object()) continue;

            OperationalEnvironment oe;
            oe.oeName = oeJson.value("name", "");
            oe.oePath = oeJson.value("path", "");

            // Validate the OE JSON file exists
            fs::path oeJsonPath = fs::absolute(proj.path).parent_path() / oe.oePath;
            if (!fs::exists(oeJsonPath)) {
                std::cerr << "Warning: OE JSON file does not exist: " << oeJsonPath << "\n";
                continue; // skip this OE
            }

            // Optional: check that the name inside the oe.json matches the expected name
            std::ifstream oeIn(oeJsonPath);
            if (oeIn) {
                try {
                    nlohmann::json oeFileJson;
                    oeIn >> oeFileJson;
                    std::string nameInFile = oeFileJson.value("name", "");
                    if (nameInFile != oe.oeName) {
                        std::cerr << "Warning: OE name mismatch in " << oeJsonPath 
                                << " (expected: " << oe.oeName << ", found: " << nameInFile << ")\n";
                        continue; // skip this OE
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to parse OE JSON file: " << oeJsonPath 
                            << " (" << e.what() << ")\n";
                    continue;
                }
            } else {
                std::cerr << "Warning: Could not open OE JSON file: " << oeJsonPath << "\n";
                continue;
            }

            proj.operationalEnvironments.push_back(oe);
        }
    }

    return proj;
}

void DataManager::SaveProject(Project& project, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;

    UpdateOEsForProject(project);

    fs::path projectJsonPath = project.path;

    // Build JSON template similar to NewProject
    nlohmann::json projectJson;
    projectJson["name"] = project.name;
    projectJson["path"] = fs::relative(projectJsonPath, fs::current_path()).string();
    projectJson["currentTabIndex"] = 0;
    projectJson["hasJentHeuristic"] = true;

    // Serialize operational environments from the Project vector
    projectJson["operationalEnvironments"] = nlohmann::json::array();
    for (const auto& oe : project.operationalEnvironments) {
        nlohmann::json oeJson;
        oeJson["name"] = oe.oeName;
        oeJson["path"] = oe.oePath;
        projectJson["operationalEnvironments"].push_back(oeJson);
    }

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

void DataManager::AddOEToProject(Project& project, const std::string& oeName, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;

    fs::path projectJsonPath = project.path;
    fs::path projectDir = projectJsonPath.parent_path();

    // Load existing project.json
    nlohmann::json projectJson;
    {
        std::ifstream in(projectJsonPath);
        if (in.is_open()) {
            try {
                in >> projectJson;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse project JSON: " << e.what() << std::endl;
            }
        }
    }

    // Ensure operationalEnvironments array exists
    if (!projectJson.contains("operationalEnvironments") || !projectJson["operationalEnvironments"].is_array()) {
        projectJson["operationalEnvironments"] = nlohmann::json::array();
    }

    // Create new OE entry
    OperationalEnvironment newOE;
    newOE.oeName = oeName;
    newOE.oePath = "OE/" + oeName + "/oe.json"; // placeholder path

    // Append to JSON for saving
    nlohmann::json jsonOE;
    jsonOE["name"] = newOE.oeName;
    jsonOE["path"] = newOE.oePath;
    projectJson["operationalEnvironments"].push_back(jsonOE);

    // Write updated project.json
    std::ofstream out(projectJsonPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open project file for writing: " << projectJsonPath << std::endl;
        return;
    }
    out << projectJson.dump(4);
    out.close();

    // Update in-memory Project object
    project.operationalEnvironments.push_back(newOE);

    // Create OE directory
    fs::path oeDir = projectDir / "OE" / oeName;
    std::error_code ec;
    fs::create_directories(oeDir, ec);
    if (ec) {
        std::cerr << "Failed to create OE directory: " << oeDir << " (" << ec.message() << ")\n";
    }

    // Create minimal oe.json
    fs::path oeJsonPath = oeDir / "oe.json";
    nlohmann::json oeJson;
    oeJson["name"] = oeName;

    std::ofstream oeOut(oeJsonPath);
    if (!oeOut.is_open()) {
        std::cerr << "Failed to create OE JSON file: " << oeJsonPath << std::endl;
    } else {
        oeOut << oeJson.dump(4);
    }

    // Optionally update appConfig.savedProjects if project not present
    auto it = std::find_if(
        appConfig.savedProjects.begin(),
        appConfig.savedProjects.end(),
        [&](const Project& p) { return p.path == project.path; }
    );

    if (it == appConfig.savedProjects.end()) {
        appConfig.savedProjects.push_back(project);
    }

    // Persist app config
    saveAppConfig("../../data/app.json", appConfig);
}

void DataManager::DeleteOE(Project& project, int oeIndex, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;
    if (oeIndex < 0 || oeIndex >= (int)project.operationalEnvironments.size()) return;

    // Identify the OE
    OperationalEnvironment oeToDelete = project.operationalEnvironments[oeIndex];

    fs::path projectJsonPath = project.path;
    fs::path projectDir = projectJsonPath.parent_path();

    // Remove from in-memory vector
    project.operationalEnvironments.erase(project.operationalEnvironments.begin() + oeIndex);

    // Update project.json
    nlohmann::json projectJson;
    {
        std::ifstream in(projectJsonPath);
        if (in.is_open()) {
            try {
                in >> projectJson;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse project JSON: " << e.what() << std::endl;
            }
        }
    }

    if (projectJson.contains("operationalEnvironments") && projectJson["operationalEnvironments"].is_array()) {
        // Erase the OE entry with matching name
        auto& oeArray = projectJson["operationalEnvironments"];
        for (auto it = oeArray.begin(); it != oeArray.end(); ++it) {
            if (it->contains("name") && (*it)["name"] == oeToDelete.oeName) {
                oeArray.erase(it);
                break;
            }
        }
    }

    // Write updated project.json
    std::ofstream out(projectJsonPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open project file for writing: " << projectJsonPath << std::endl;
        return;
    }
    out << projectJson.dump(4);
    out.close();

    // Delete OE directory on disk
    fs::path oeDir = projectDir / ("OE/" + oeToDelete.oeName);
    std::error_code ec;
    fs::remove_all(oeDir, ec);
    if (ec) {
        std::cerr << "Failed to remove OE directory: " << oeDir << " (" << ec.message() << ")\n";
    }

    // After removing OE, check if "OE" directory is empty
    fs::path oeRootDir = projectDir / "OE";
    if (fs::exists(oeRootDir) && fs::is_directory(oeRootDir)) {
        bool empty = fs::is_empty(oeRootDir, ec);
        if (!ec && empty) {
            fs::remove(oeRootDir, ec);
            if (ec) {
                std::cerr << "Failed to remove empty OE root directory: " << oeRootDir << " (" << ec.message() << ")\n";
            }
        }
    }

    // Persist app config (make sure project is still in savedProjects)
    auto it = std::find_if(
        appConfig.savedProjects.begin(),
        appConfig.savedProjects.end(),
        [&](const Project& p) { return p.path == project.path; }
    );

    if (it == appConfig.savedProjects.end()) {
        appConfig.savedProjects.push_back(project);
    } else {
        *it = project; // update existing
    }

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

void DataManager::UpdateOEsForProject(Project& project) {
    fs::path projectDir = fs::path(project.path).parent_path();
    fs::path oeParentDir = projectDir / "OE";

    // Ensure the parent OE folder exists
    if (!fs::exists(oeParentDir)) {
        fs::create_directories(oeParentDir);
    }

    for (auto& oe : project.operationalEnvironments) {
        fs::path oldDir = projectDir / oe.oePath;   // full relative path from project
        oldDir = oldDir.parent_path();   
        fs::path newDir = oeParentDir / oe.oeName;

        if (fs::exists(oldDir) && oldDir != newDir) {
            try {
                fs::rename(oldDir, newDir);
            } catch (const std::exception& e) {
                std::cerr << "Failed to rename OE directory: " << e.what() << std::endl;
            }
        }

        // Update oe.json in new directory
        nlohmann::json oeJson;
        oeJson["name"] = oe.oeName;
        fs::path oeJsonPath = newDir / "oe.json";
        {
            std::ofstream out(oeJsonPath);
            if (out.is_open()) out << oeJson.dump(4);
        }

        // Update path in project.json relative to projectDir
        oe.oePath = fs::relative(oeJsonPath, projectDir).string();
    }
}

