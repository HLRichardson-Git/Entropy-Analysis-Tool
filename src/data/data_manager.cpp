
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
        *currentProject = LoadProject(config->lastOpenedProject.path + "\\project.json");
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
    projectTemplate["vendor"] = vendor;
    projectTemplate["repo"] = repo;
    projectTemplate["name"] = projectName;
    projectTemplate["path"] = fs::relative(projectDir, fs::current_path()).string();
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

    // Project name
    proj.vendor = j.value("vendor", fullPath.stem().string());
    proj.repo = j.value("repo", fullPath.stem().string());
    proj.name = j.value("name", fullPath.stem().string());
    proj.path = fullPath.parent_path().string();

    proj.operationalEnvironments.clear();
    if (j.contains("operationalEnvironments") && j["operationalEnvironments"].is_array()) {
        for (auto& oeJson : j["operationalEnvironments"]) {
            if (!oeJson.is_object()) continue;

            OperationalEnvironment oe;
            oe.oeName = oeJson.value("name", "");
            oe.oePath = oeJson.value("path", "");

            fs::path oeJsonPath = fs::absolute(proj.path) / oe.oePath / "oe.json";
            if (!fs::exists(oeJsonPath)) {
                std::cerr << "Warning: OE JSON file does not exist: " << oeJsonPath << "\n";
                continue;
            }

            std::ifstream oeIn(oeJsonPath);
            if (!oeIn) {
                std::cerr << "Warning: Could not open OE JSON file: " << oeJsonPath << "\n";
                continue;
            }

            try {
                nlohmann::json oeFileJson;
                oeIn >> oeFileJson;

                std::string nameInFile = oeFileJson.value("name", "");
                if (nameInFile != oe.oeName) {
                    std::cerr << "Warning: OE name mismatch in " << oeJsonPath 
                              << " (expected: " << oe.oeName << ", found: " << nameInFile << ")\n";
                    continue;
                }

                if (oeFileJson.contains("heuristicData") && oeFileJson["heuristicData"].is_object()) {
                    auto& heuristicJson = oeFileJson["heuristicData"];
                    auto& mainHist = oe.heuristicData.mainHistogram;

                    if (heuristicJson.contains("mainHistogram") && heuristicJson["mainHistogram"].is_object()) {
                        auto& mhJson = heuristicJson["mainHistogram"];
                        mainHist.heuristicFilePath = mhJson.value("heuristicFilePath", "");
                        mainHist.convertedFilePath = mhJson.value("convertedFilePath", "");
                        mainHist.minValue = mhJson.value("minValue", 0u);
                        mainHist.maxValue = mhJson.value("maxValue", 0u);
                        mainHist.binWidth  = mhJson.value("binWidth", 1.0);

                        if (mhJson.contains("computedBins") && mhJson["computedBins"].is_array()) {
                            mainHist.binCounts.fill(0);
                            size_t i = 0;
                            for (auto& bin : mhJson["computedBins"]) {
                                if (bin.is_number_integer() && i < mainHist.binCounts.size()) {
                                    mainHist.binCounts[i++] = bin.get<int>();
                                }
                            }
                        }

                        if (mhJson.contains("nonIidResults") && mhJson["nonIidResults"].is_object()) {
                            auto& resJson = mhJson["nonIidResults"];
                            auto& res = mainHist.entropyResults;
                            res.H_original  = resJson.value("H_original", res.H_original.value_or(0.0));
                            res.H_bitstring = resJson.value("H_bitstring", res.H_bitstring.value_or(0.0));
                            res.min_entropy = resJson.value("min_entropy", res.min_entropy.value_or(0.0));
                        }

                        mainHist.firstPassingDecimationResult = mhJson.value("firstPassingDecimationResult", "");
                    }

                    // Load subHistograms
                    if (heuristicJson.contains("subHistograms") && heuristicJson["subHistograms"].is_array()) {
                        for (auto& shJson : heuristicJson["subHistograms"]) {
                            if (!shJson.is_object()) continue;

                            SubHistogram sh;
                            sh.rect.X.Min = shJson.value("min", 0.0);
                            sh.rect.X.Max = shJson.value("max", sh.rect.X.Min);
                            sh.rect.Y.Min = 0.0;
                            sh.rect.Y.Max = 0.0;

                            if (shJson.contains("color") && shJson["color"].is_array() && shJson["color"].size() == 4) {
                                sh.color.x = shJson["color"][0].get<float>();
                                sh.color.y = shJson["color"][1].get<float>();
                                sh.color.z = shJson["color"][2].get<float>();
                                sh.color.w = shJson["color"][3].get<float>();
                            } else {
                                sh.color = ImVec4(0.84f, 0.28f, 0.28f, 0.25f);
                            }

                            sh.minValue = shJson.value("min", 0u);
                            sh.maxValue = shJson.value("max", 0u);

                            if (shJson.contains("nonIidResults") && shJson["nonIidResults"].is_object()) {
                                auto& rJson = shJson["nonIidResults"];
                                auto& rRes = sh.entropyResults;
                                rRes.H_original  = rJson.value("H_original", rRes.H_original.value_or(0.0));
                                rRes.H_bitstring = rJson.value("H_bitstring", rRes.H_bitstring.value_or(0.0));
                                rRes.min_entropy = rJson.value("min_entropy", rRes.min_entropy.value_or(0.0));
                            }

                            sh.subHistIndex = static_cast<int>(oe.heuristicData.mainHistogram.subHists.size()) + 1;
                            oe.heuristicData.mainHistogram.subHists.push_back(std::move(sh));
                        }
                    }
                }

            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to parse OE JSON: " << oeJsonPath << " (" << e.what() << ")\n";
                continue;
            }

            proj.operationalEnvironments.push_back(std::move(oe));
        }
    }

    return proj;
}

void DataManager::SaveProject(Project& project, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;

    UpdateOEsForProject(project);

    fs::path projectDir = project.path;
    fs::path projectJsonPath = projectDir / "project.json";

    // Build JSON template similar to NewProject
    nlohmann::json projectJson;
    projectJson["vendor"] = project.vendor;
    projectJson["repo"] = project.repo;
    projectJson["name"] = project.name;
    projectJson["path"] = fs::relative(projectDir, fs::current_path()).string();
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
    } else {
        *it = project; // ensure vendor/repo are up-to-date
    }

    // Persist app.json
    saveAppConfig("../../data/app.json", appConfig);
}

void DataManager::AddOEToProject(Project& project, const std::string& oeName, Config::AppConfig& appConfig) {
    if (project.name.empty() || project.path.empty()) return;

    fs::path projectDir = project.path;
    fs::path projectJsonPath = projectDir / "project.json";

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
    newOE.oePath = "OE/" + oeName;

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

    fs::path projectDir = project.path;
    fs::path projectJsonPath = projectDir / "project.json";

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
    fs::path oeDir = projectDir / oeToDelete.oePath;
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
    fs::path projectDir = fs::path(project.path);
    fs::path oeParentDir = projectDir / "OE";
    if (!fs::exists(oeParentDir)) fs::create_directories(oeParentDir);

    for (auto& oe : project.operationalEnvironments) {
        fs::path oldDir = projectDir / oe.oePath;
        fs::path newDir = oeParentDir / oe.oeName;

        if (fs::exists(oldDir) && oldDir != newDir) {
            try {
                fs::rename(oldDir, newDir);
                // Update main histogram file path if needed
                auto& mainHist = oe.heuristicData.mainHistogram;
                if (!mainHist.heuristicFilePath.empty()) {
                    fs::path oldPath = mainHist.heuristicFilePath;
                    if (oldPath.string().find(oldDir.string()) == 0) {
                        fs::path rel = fs::relative(oldPath, oldDir);
                        mainHist.heuristicFilePath = (newDir / rel).string();
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to rename OE directory: " << e.what() << std::endl;
            }
        }

        nlohmann::json oeJson;
        oeJson["name"] = oe.oeName;

        nlohmann::json heuristicJson;
        auto& mainHist = oe.heuristicData.mainHistogram;

        if (!mainHist.heuristicFilePath.empty()) {
            nlohmann::json mhJson;
            mhJson["heuristicFilePath"] = mainHist.heuristicFilePath;
            mhJson["convertedFilePath"] = mainHist.convertedFilePath;

            mhJson["minValue"] = mainHist.minValue;
            mhJson["maxValue"] = mainHist.maxValue;
            mhJson["binWidth"] = mainHist.binWidth;
            mhJson["computedBins"] = mainHist.binCounts;

            if (mainHist.entropyResults.min_entropy.has_value()) {
                auto& res = mainHist.entropyResults;
                nlohmann::json resJson;
                if (res.H_original.has_value())  resJson["H_original"]  = res.H_original.value();
                if (res.H_bitstring.has_value()) resJson["H_bitstring"] = res.H_bitstring.value();
                resJson["min_entropy"] = res.min_entropy.value();
                mhJson["nonIidResults"] = resJson;
            }

            if (!mainHist.firstPassingDecimationResult.empty())
                mhJson["firstPassingDecimationResult"] = mainHist.firstPassingDecimationResult;

            // Save subHistograms
            nlohmann::json subHistJson = nlohmann::json::array();
            for (auto& sh : oe.heuristicData.mainHistogram.subHists) {
                nlohmann::json j;
                j["min"] = sh.rect.X.Min;
                j["max"] = sh.rect.X.Max;
                j["color"] = { sh.color.x, sh.color.y, sh.color.z, sh.color.w };
                if (sh.entropyResults.min_entropy.has_value()) {
                    auto& res = sh.entropyResults;
                    nlohmann::json rj;
                    if (res.H_original.has_value())  rj["H_original"]  = res.H_original.value();
                    if (res.H_bitstring.has_value()) rj["H_bitstring"] = res.H_bitstring.value();
                    rj["min_entropy"] = res.min_entropy.value();
                    j["nonIidResults"] = rj;
                }
                subHistJson.push_back(j);
            }

            heuristicJson["mainHistogram"] = mhJson;
            heuristicJson["subHistograms"] = subHistJson;
        }

        oeJson["heuristicData"] = heuristicJson;

        fs::path oeJsonPath = newDir / "oe.json";
        std::ofstream out(oeJsonPath);
        if (out.is_open()) out << oeJson.dump(4);

        oe.oePath = fs::relative(newDir, projectDir).string();
    }
}

// Heuristic
void DataManager::processHistogramForProject(Project& project, int oeIndex, ThreadPool& pool, NotificationCallback notify) {
    auto* oePtr = &project.operationalEnvironments[oeIndex];

    pool.Enqueue([oePtr, notify]() {
        if (notify) notify("Processing histogram...", 5.0f, ImVec4(0.1f, 0.7f, 1.0f, 1.0f));

        auto filePath = oePtr->heuristicData.mainHistogram.heuristicFilePath;
        MainHistogram hist = computeHistogramFromFile(filePath);
        hist.heuristicFilePath = filePath; // preserve
        hist.convertedFilePath  = oePtr->heuristicData.mainHistogram.convertedFilePath;   // preserve converted path
        oePtr->heuristicData.mainHistogram = std::move(hist);

        if (notify) notify("Histogram processing complete!", 5.0f, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
    });
}

bool DataManager::ConvertDecimalFile(
    const std::filesystem::path& inputFilePath,
    lib90b::EntropyInputData& outData,
    std::string& outBinaryFilePath,
    std::optional<double> minVal,
    std::optional<double> maxVal,
    int regionIndex)
{
    std::ifstream inFile(inputFilePath);
    if (!inFile.is_open()) return false;

    std::vector<uint8_t> symbols;
    std::string line;

    while (std::getline(inFile, line)) {
        try {
            double val = std::stod(line);

            // If range is provided, skip values outside it
            if ((minVal && val < minVal.value()) || (maxVal && val > maxVal.value())) {
                continue;
            }

            uint64_t sample = static_cast<uint64_t>(val * 1e6); // scale if needed
            uint8_t symbol = static_cast<uint8_t>(sample & 0xFF);
            symbols.push_back(symbol);
        } catch (...) {
            continue; // skip invalid lines
        }
    }

    if (symbols.empty()) return false;

    // Fill EntropyInputData
    outData.symbols = std::move(symbols);
    outData.word_size = 8;
    outData.alph_size = 256;

    // Prepare output file path
    fs::path inputPath(inputFilePath);
    std::string stem = inputPath.stem().string();
    std::string outFileName = stem;
    if (regionIndex > 0) outFileName += "_region" + std::to_string(regionIndex);
    fs::path outPath = inputPath.parent_path() / (outFileName + ".bin");

    // Save binary file
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(reinterpret_cast<const char*>(outData.symbols.data()), outData.symbols.size());
    outFile.close();

    outBinaryFilePath = outPath.string();
    return true;
}
