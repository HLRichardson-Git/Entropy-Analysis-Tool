
#include "heuristic_manager.h"

bool HeuristicManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project) {
    m_dataManager = dataManager;
    m_config = config;
    m_currentProject = project;
    
    return true;
}

void HeuristicManager::Render() {
    if (auto file = FileSelector("HistogramFileDlg", "Load Raw Samples", ".data,.bin,.txt,.*")) {
        std::string path = *file;
    }
}