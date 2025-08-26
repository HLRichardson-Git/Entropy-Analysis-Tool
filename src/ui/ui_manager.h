
#pragma once

#include <functional>
#include <memory>

#include "../data/data_manager.h"
#include "../core/types.h"

class UIManager {
private:
    DataManager* m_dataManager;
    UIState& uiState;

public:
    bool newProjectRequested = false;

    UIManager(UIState& state) : uiState(state), m_dataManager(nullptr) {}
    ~UIManager() = default;
    
    bool Initialize(DataManager* dataManager);
    void Render(Tabs& activeTab, Project& project);

    std::function<void()> onNewProject;
    std::function<void()> onOpenProject;
    std::function<void()> onSaveProject;
    std::function<void()> onToggleFullscreen;
    std::function<void()> onShowHelp;

    void RenderSidebar(Tabs& activeTab, Project& project);
    void RenderMainContent(Tabs& activeTab, Project& project);

    void RenderMenuBar();
    void RenderMainWindow(Tabs& activeTab, Project& project);
    void RenderHelpWindow(bool show_help_window);

    NewProjectFormResult RenderNewProjectPopup(const std::vector<std::string>& vendors);
    LoadProjectFormResult RenderLoadProjectPopup(const std::vector<Project>& savedProjects);
};