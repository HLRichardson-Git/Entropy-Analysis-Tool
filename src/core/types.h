
#pragma once

#include "../data/histogram/histogram.h"

#include <future>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>
#include <lib90b/entropy_tests.h>
#include <lib90b/non_iid.h>

enum class Tabs {
    StatisticalAssessment,
    HeuristicAssessment
};

struct HistogramRegion {
    ImPlotRect rect;   // the draggable rectangle
    ImVec4 color;      // user-selected color
    bool active = true;

    lib90b::EntropyInputData entropyData;
    lib90b::NonIidResult entropyResults;
    std::string convertedFilePath;

    std::string subFilePath;
    int regionIndex = 0;

    bool testsRunning = false;
    std::chrono::steady_clock::time_point startTime;
};

struct HeuristicData {
    std::string heuristicFilePath;
    std::string convertedFilePath;

    PrecomputedHistogram mainHistogram;
    std::vector<HistogramRegion> regions;

    lib90b::EntropyInputData entropyData;
    lib90b::NonIidResult entropyResults;

    bool testsRunning = false;
    std::chrono::steady_clock::time_point startTime;
};

struct OperationalEnvironment {
    std::string oeName;
    std::string oePath;

    HeuristicData heuristicData;
};

struct Project {
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
