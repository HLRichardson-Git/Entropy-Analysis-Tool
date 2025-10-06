
#pragma once

#include <implot.h>

#include "../../data/data_manager.h"
#include "../../data/histogram/histogram.h"
#include "../../core/types.h"
#include "../../core/app_command/app_command.h"
#include "../../file_utils/file_utils.h"

using CommandCallback = std::function<void(AppCommand)>;
using NotificationCallback = std::function<void(const std::string&, float, ImVec4)>;

class HeuristicManager {
private:
    DataManager* m_dataManager;
    Config::AppConfig* m_config;
    Project* m_currentProject;

    UIState* m_uiState;
    CommandCallback m_onCommand;
    NotificationCallback m_onNotification;

    OperationalEnvironment* GetSelectedOE();

    bool m_editHistogramPopupOpen = false;

    void StartHistogramProcessing(const fs::path& filePath);

    // Popups
    void RenderPopups();
    void RenderUploadSectionForOE(OperationalEnvironment* oe);
    void RenderMainHistogramConfigPopup();

public:

    HeuristicManager() = default;
    ~HeuristicManager() = default;
    
    bool Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState);
    void Render();

    bool m_showBatchPopup = false;
    void RenderBatchHeuristic();

    void SetCommandCallback(CommandCallback cb) { m_onCommand = cb; };
    void SetNotificationCallback(NotificationCallback cb) { m_onNotification = cb; }

    void SetCurrentProject(Project* project) { m_currentProject = project; }
};