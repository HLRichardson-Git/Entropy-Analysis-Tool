
#include "heuristic_manager.h"

bool HeuristicManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState) {
    m_dataManager = dataManager;
    m_config = config;
    SetCurrentProject(project);
    m_uiState = uiState;
    
    return true;
}

void HeuristicManager::Render() {
    if (!m_currentProject) return;

    if (auto oe = GetSelectedOE()) {
        ImGui::Text("Current OE: %s", oe->oeName.c_str());

        // File selection button
        if (auto file = FileSelector("HistogramFileDlg", "Load Raw Samples", ".data,.bin,.txt,.*")) {
            fs::path destDir = fs::path(m_currentProject->path) / oe->oePath;
            if (auto dest = CopyFileToDirectory(*file, destDir)) {
                // Store copied file path in the OE
                oe->heuristicData.heuristicFilePath = dest->string();
            }
        }
        ImGui::SameLine();
        
        // Show the copied file path (if any)
        if (!oe->heuristicData.heuristicFilePath.empty()) {
            ImGui::TextWrapped("Current file: %s", oe->heuristicData.heuristicFilePath.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file loaded for this OE");
        }
    }
}

OperationalEnvironment* HeuristicManager::GetSelectedOE() {
    if (!m_currentProject || !m_uiState) return nullptr;
    int idx = m_uiState->selectedOEIndex;
    if (idx < 0 || idx >= (int)m_currentProject->operationalEnvironments.size())
        return nullptr;
    return &m_currentProject->operationalEnvironments[idx];
}
