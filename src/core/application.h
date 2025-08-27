
#pragma once

#include "../data/data_manager.h"
#include "../ui/ui_manager.h"
#include "app_command/app_command.h"
#include "types.h"
#include "config.h"

class Application {
private:
    DataManager dataManager;
    CommandQueue commandQueue;
    UIManager uiManager{commandQueue};

    Config::AppConfig config;
    Project currentProject;
    std::vector<std::string> vendors;
    
    void SetupImGuiStyle();
    void LoadFonts();

public:
    Application() {}
    ~Application() = default;
    
    // Lifecycle
    bool Initialize();
    void Update();
    void Render();
    void Shutdown();
    
    // Project management
    void NewProject(const NewProjectCommand& formResult);
    bool LoadProject(const std::string& filePath);
    void SaveProject();

    void ShowHelp();
};