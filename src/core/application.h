
#pragma once

#include "../data/data_manager.h"
#include "../ui/ui_manager.h"
#include "types.h"
#include "config.h"

class Application {
private:
    DataManager dataManager;
    UIState uiState;
    UIManager uiManager{uiState};

    Config::AppConfig config;
    Project currentProject;

    bool show_help_window;
    Tabs activeTab = Tabs::StatisticalAssessment;
    
    void SetupImGuiStyle();
    void LoadFonts();

public:
    Application() : show_help_window(false) {}
    ~Application() = default;
    
    // Lifecycle
    bool Initialize();
    void Update();
    void Render();
    void Shutdown();
    
    // User actions
    void SaveProject();
    bool OpenProject(const std::string& filePath);
    void NewProject();
    bool LoadProject(const std::string& filename);
    void ShowHelp();
};