
#pragma once

#include <functional>
#include <memory>

#include "../data/data_manager.h"
#include "heuristic_assessment/heuristic_manager.h"
#include "../core/types.h"
#include "../core/app_command/app_command.h"

class UIManager {
private:
    DataManager* m_dataManager;
    Config::AppConfig* m_config;
    Project* m_currentProject;

    UIState uiState;
    CommandQueue& commandQueue;
    std::vector<Notification> notifications;

    HeuristicManager heuristicManager;

    // Main Content
    void RenderMainWindow();
    void RenderSidebar();
    void RenderMainContent();

    // Render Utility
    void RenderMenuBar();
    void RenderHelpWindow();

    // Notifications
    void PushNotification(const std::string& msg, float duration = 3.0f, ImVec4 color = ImVec4(1,1,1,1));
    void RenderNotifications();

    // Popups
    void RenderPopups();
    NewProjectFormResult RenderNewProjectPopup();
    LoadProjectFormResult RenderLoadProjectPopup();
    AddOEFormResult RenderAddOEPopup();
    EditOEFormResult RenderEditOEPopup();

    // UI elements
    void ImGuiSpacing(int count = 1);

public:

    UIManager(CommandQueue& queue) : commandQueue(queue), m_dataManager(nullptr), m_config(nullptr),  m_currentProject(nullptr) {}
    ~UIManager() = default;
    
    bool Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project);
    void Render();

    // Utility
    void OnProjectChanged(Project project);
};