
#pragma once

#include <implot.h>

#include "../../data/data_manager.h"
#include "../../core/types.h"
#include "../../core/app_command/app_command.h"
#include "../../file_utils/file_utils.h"

using CommandCallback = std::function<void(AppCommand)>;
using NotificationCallback = std::function<void(const std::string&, float, ImVec4)>;

enum class StatisticTabs {
    Summary,
    FullOutput
};

class StatisticManager {
private:
    DataManager* m_dataManager;
    Config::AppConfig* m_config;
    Project* m_currentProject;

    UIState* m_uiState;
    CommandCallback m_onCommand;
    NotificationCallback m_onNotification;

    OperationalEnvironment* GetSelectedOE();

    StatisticTabs nonIidTab = StatisticTabs::Summary;
    StatisticTabs restartTab = StatisticTabs::Summary;

    //void RenderUploadSectionForOE(OperationalEnvironment* oe);
    void RenderUploadSectionForOE(
        OperationalEnvironment* oe,
        std::filesystem::path& filePathVar,
        const std::string& labelPrefix,
        const std::string& fileExtensions,
        const ImVec2& buttonSize = ImVec2(0, 0), 
        const Config::ButtonPalette& buttonColor = Config::GREY_BUTTON,
        const ImVec4& textColor = Config::TEXT_DARK_CHARCOAL
    );

    // Popups
    void RenderPopups();

public:

    StatisticManager() = default;
    ~StatisticManager() = default;
    
    bool Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState);
    void Render();

    bool m_showBatchPopup = false;
    void RenderBatchStatistic();

    void SetCommandCallback(CommandCallback cb) { m_onCommand = cb; };
    void SetNotificationCallback(NotificationCallback cb) { m_onNotification = cb; }

    void SetCurrentProject(Project* project) { m_currentProject = project; }
};