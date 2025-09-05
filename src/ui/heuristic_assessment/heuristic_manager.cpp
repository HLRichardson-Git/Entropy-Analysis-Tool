
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

    float fullWidth = ImGui::GetContentRegionAvail().x;

    float sidebarWidth = fullWidth * 0.20f;
    float mainWidth    = fullWidth * 0.80f;

    auto oe = GetSelectedOE();

    // Sidebar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("SelectMainHistogramRegions", ImVec2(sidebarWidth, 360), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string selectMainHistogramRegionsTitle = std::string(u8"\uf0fe") + "  Selected Regions";
        ImGui::Text(selectMainHistogramRegionsTitle.c_str());
        ImGui::PopFont();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Main content
    ImGui::BeginChild("MainHistogram", ImVec2(mainWidth - 10.0f, 360), true);
    {
        // Text on the left
        ImGui::PushFont(Config::fontH3_Bold);
        std::string mainHeuristicTitle = std::string(u8"\uf1fe") + "  Main Histogram - " + oe->oeName;
        ImGui::Text(mainHeuristicTitle.c_str());

        // Calculate position for right-aligned buttons
        ImVec2 cogSize(30, 30);
        ImVec2 statisticalTestSize(200, 30); // wider button with text
        float spacing = 8.0f;

        // Total width for both buttons + spacing
        float totalWidth = statisticalTestSize.x + spacing + cogSize.x;

        // Align buttons as a group to the right
        float regionWidth = ImGui::GetContentRegionAvail().x;
        float startX = regionWidth - totalWidth + 5.0f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(startX);

        ImGui::PushFont(Config::fontH3);

        // Run Statistical Tests button
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::MINT_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::MINT_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::MINT_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
        {
            std::string runStatisticalTestButton = std::string(u8"\uf83e") + "  Run Statistical Tests";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                
            }
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, spacing);

        // Cog button
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::WHITE_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::WHITE_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::WHITE_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
        {
            std::string cogButton = std::string(u8"\uf013");
            if (ImGui::Button(cogButton.c_str(), cogSize)) {
                m_editHistogramPopupOpen = true;
            }
            ImGui::PopFont();
        }
        ImGui::PopStyleColor(4);

        ImGui::PopFont();

        // Placeholder histogram using ImPlot
        if (ImPlot::BeginPlot("##MainHistogramPlot", ImVec2(-1, -1))) {
            // Dummy data for now
            static float values[50];
            static bool initialized = false;
            if (!initialized) {
                for (int i = 0; i < 50; ++i) {
                    values[i] = static_cast<float>(rand() % 100) / 100.0f; // random 0–1
                }
                initialized = true;
            }

            ImPlot::SetupAxes("Bin", "Frequency");
            ImPlot::PlotHistogram("Entropy", values, 50, 10); // 50 bins, scale = 10

            ImPlot::EndPlot();
        }

    }
    ImGui::EndChild();

    ImGui::PopStyleColor();

    RenderPopups();
}

OperationalEnvironment* HeuristicManager::GetSelectedOE() {
    if (!m_currentProject || !m_uiState) return nullptr;
    int idx = m_uiState->selectedOEIndex;
    if (idx < 0 || idx >= (int)m_currentProject->operationalEnvironments.size())
        return nullptr;
    return &m_currentProject->operationalEnvironments[idx];
}

void HeuristicManager::RenderPopups() { 
    if (m_editHistogramPopupOpen) {
        RenderMainHistogramConfigPopup();
    }
}

void HeuristicManager::RenderMainHistogramConfigPopup() {
    // Keep calling OpenPopup every frame until modal is shown
    if (m_editHistogramPopupOpen) {
        ImGui::OpenPopup("Edit Main Histogram");
    }

    ImVec2 minSize(400, 200);  // minimum size
    ImVec2 maxSize(FLT_MAX, FLT_MAX); // no real max
    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

    if (ImGui::BeginPopupModal("Edit Main Histogram", nullptr)) {
        auto oe = GetSelectedOE();
        std::string editMainHeuristicTitle = "Edit Main Histogram - " + oe->oeName;
        ImGui::Text(editMainHeuristicTitle.c_str());

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
            fs::path filePath(oe->heuristicData.heuristicFilePath);
            std::string filename = filePath.filename().string();

            ImGui::TextWrapped("Current file: %s", filename.c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(oe->heuristicData.heuristicFilePath.c_str());
                ImGui::EndTooltip();
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file loaded for this OE");
        }

        if (ImGui::Button("Process uploaded file")) {
            m_dataManager->processHistogramForProject(*m_currentProject, m_uiState->selectedOEIndex);
        }

        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 buttonSize = ImVec2(120, 0); // width, auto height

        // Calculate Y so the button sits at the bottom, with some padding
        float bottomY = windowSize.y - ImGui::GetStyle().WindowPadding.y - buttonSize.y - 20.0f;

        ImGui::SetCursorPosY(bottomY);

        if (ImGui::Button("Cancel", buttonSize)) {
            ImGui::CloseCurrentPopup();
            m_editHistogramPopupOpen = false; // reset flag
        }

        ImGui::EndPopup();
    }
}
