
#pragma once

#include "../../data/data_manager.h"
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

public:

    HeuristicManager() = default;
    ~HeuristicManager() = default;
    
    bool Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState);
    void Render();

    void SetCommandCallback(CommandCallback cb) { m_onCommand = cb; };
    void SetNotificationCallback(NotificationCallback cb) { m_onNotification = cb; }

    void SetCurrentProject(Project* project) { m_currentProject = project; }
};