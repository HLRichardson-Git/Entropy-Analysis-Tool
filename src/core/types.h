
#pragma once

//#include "../data/histogram/histogram.h"

#include <future>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <regex>

#include <imgui.h>
#include <implot.h>
#include <lib90b/entropy_tests.h>
#include <lib90b/non_iid.h>

enum class Tabs {
    StatisticalAssessment,
    HeuristicAssessment
};

struct TestTimer {
    bool testRunning = false;
    std::chrono::steady_clock::time_point testStartTime;

    void StartTestsTimer() {
        testRunning = true;
        testStartTime = std::chrono::steady_clock::now();
    }

    void StopTestsTimer() {
        testRunning = false;
    }
};

struct NonIidParsedResults {
    double minEntropy = 0.0f;
    double h_original = 0.0f;
    double h_bitstring = 0.0f;

    bool ParseResult(const std::string& rawResultsOutput) {
        // Extract H_original
        std::regex h_original_pattern(R"(H_original:\s*([\d.]+))");
        std::smatch h_original_match;
        if (std::regex_search(rawResultsOutput, h_original_match, h_original_pattern)) {
            try {
                h_original = std::stod(h_original_match[1]);
            } catch (...) {
                return false;
            }
        } else {
            return false;
        }
        
        // Extract H_bitstring
        std::regex h_bitstring_pattern(R"(H_bitstring:\s*([\d.]+))");
        std::smatch h_bitstring_match;
        if (std::regex_search(rawResultsOutput, h_bitstring_match, h_bitstring_pattern)) {
            try {
                h_bitstring = std::stod(h_bitstring_match[1]);
            } catch (...) {
                return false;
            }
        } else {
            return false;
        }
        
        // Extract min entropy
        std::regex min_entropy_pattern(R"(min\(H_original, (?:\d+) X H_bitstring\):\s*([\d.]+))");
        std::smatch min_entropy_match;
        if (std::regex_search(rawResultsOutput, min_entropy_match, min_entropy_pattern)) {
            try {
                minEntropy = std::stod(min_entropy_match[1]);
            } catch (...) {
                return false;
            }
        } else {
            return false;
        }
        
        return true;
    }
};

struct StatisticData {
    std::filesystem::path nonIidSampleFilePath;
    std::filesystem::path restartSampleFilePath;

    std::filesystem::path nonIidResultFilePath;
    std::filesystem::path restartResultFilePath;

    std::string nonIidResult = "";
    std::string restartResult = "";

    NonIidParsedResults nonIidParsedResults;

    TestTimer nonIidTestTimer;
    TestTimer restartTestTimer;
};

struct BaseHistogram {
    static constexpr int binCount = 1500;
    unsigned int minValue = 0;
    unsigned int maxValue = 0;
    double binWidth = 0.0;
    std::array<int, binCount> binCounts{};

    //lib90b::EntropyInputData entropyData;
    //lib90b::NonIidResult entropyResults;
    std::filesystem::path nonIidSampleFilePath;
    std::filesystem::path nonIidResultFilePath;
    std::string nonIidResult = "";
    NonIidParsedResults nonIidParsedResults;
    
    TestTimer testTimer;
};

struct SubHistogram : public BaseHistogram {
    ImPlotRect rect;
    ImVec4 color;
    int subHistIndex = 0;
};

struct MainHistogram : public BaseHistogram {
    std::string firstPassingDecimationResult;
    std::filesystem::path heuristicFilePath;
    std::filesystem::path convertedFilePath;

    std::vector<SubHistogram> subHists;

    TestTimer decimationTestTimer;
};

struct HeuristicData {
    MainHistogram mainHistogram;
};

struct OperationalEnvironment {
    std::string oeName;
    std::string oePath;

    StatisticData statisticData;
    HeuristicData heuristicData;
};

struct Project {
    std::string vendor;
    std::string repo;
    std::string name;
    std::string path;

    std::vector<OperationalEnvironment> operationalEnvironments;
};

struct NewProjectFormResult {
    bool submitted = false;
    std::string vendor;
    std::string repo;
    std::string projectName;
};

struct LoadProjectFormResult {
    bool submitted = false;
    std::string filePath;
};

struct AddOEFormResult {
    bool submitted = false;
    std::string oeName;
};

struct EditOEFormResult {
    bool submitted = false;
    bool deleted = false;
    std::string newName;
};

struct UIState {
    Tabs activeTab = Tabs::StatisticalAssessment;
    
    bool newProjectPopupOpen = false;
    NewProjectFormResult newProjectFormResult;

    bool loadProjectPopupOpen = false;
    LoadProjectFormResult loadProjectFormResult;

    bool saveProjectRequested = false;
    bool showSaveNotification = false; // for temporary save notification
    float saveNotificationTimer = 0.0f; // counts down in seconds

    bool addOEPopupOpen = false;
    AddOEFormResult addOEFormResult;

    bool editOEPopupOpen = false;

    int selectedOEIndex = -1; // -1 means no OE selected

    bool showFileConverterPopup = false;
    
    bool showHelpWindow = false;
};

struct Notification {
    std::string message;
    float duration;          // seconds left
    ImVec4 color;            // optional, for type
};
