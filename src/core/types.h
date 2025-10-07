
#pragma once

//#include "../data/histogram/histogram.h"

#include <future>
#include <string>
#include <vector>
#include <filesystem>

#include <imgui.h>
#include <implot.h>
#include <lib90b/entropy_tests.h>
#include <lib90b/non_iid.h>

enum class Tabs {
    StatisticalAssessment,
    HeuristicAssessment
};

struct StatisticData {
    std::filesystem::path nonIidSampleFilePath;
    std::filesystem::path restartSampleFilePath;
};

struct BaseHistogram {
    static constexpr int binCount = 1500;
    unsigned int minValue = 0;
    unsigned int maxValue = 0;
    double binWidth = 0.0;
    std::array<int, binCount> binCounts{};

    lib90b::EntropyInputData entropyData;
    lib90b::NonIidResult entropyResults;
    
    bool testsRunning = false;
    std::chrono::steady_clock::time_point startTime;

    void StartTestsTimer() {
        testsRunning = true;
        startTime = std::chrono::steady_clock::now();
    }

    void StopTestsTimer() {
        testsRunning = false;
    }

    double GetElapsedTestsSeconds() const {
        if (!testsRunning) return 0.0;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }
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

    bool decimationRunning = false;
    std::chrono::steady_clock::time_point decimationStartTime;

    void StartDecimationTimer() {
        decimationRunning = true;
        decimationStartTime = std::chrono::steady_clock::now();
    }

    void StopDecimationTimer() {
        decimationRunning = false;
    }

    double GetElapsedDecimationSeconds() const {
        if (!decimationRunning) return 0.0;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - decimationStartTime).count();
    }
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

    bool showHelpWindow = false;
};

struct Notification {
    std::string message;
    float duration;          // seconds left
    ImVec4 color;            // optional, for type
};
