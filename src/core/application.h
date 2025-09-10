
#pragma once

#include <algorithm>

#include "../data/data_manager.h"
#include "../ui/ui_manager.h"
#include "app_command/app_command.h"
#include "thread_pool/thread_pool.h"
#include "types.h"
#include "config.h"

class Application {
private:
    DataManager dataManager;
    CommandQueue commandQueue;
    UIManager uiManager{commandQueue};
    ThreadPool threadPool;

    Config::AppConfig config;
    Project currentProject;
    std::vector<std::string> vendors;
    
    void SetupImGuiStyle();
    void LoadFonts();
    ThreadPool& GetThreadPool() { return threadPool; }

public:
    //Application() {}
    Application() 
        : threadPool([]{
            unsigned int n = std::thread::hardware_concurrency();
            if (n <= 1) n = 1;
            return n - 1;
        }())
    {}
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