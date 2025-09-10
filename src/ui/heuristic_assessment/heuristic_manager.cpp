
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

    float sidebarWidth = 250.0f; // Hardcoded for simplicity
    float mainWidth = fullWidth - sidebarWidth;

    auto oe = GetSelectedOE();

    // Sidebar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("SelectMainHistogramRegions", ImVec2(sidebarWidth, 360), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string selectMainHistogramRegionsTitle = std::string(u8"\uf0fe") + "  Selected Regions";
        ImGui::Text(selectMainHistogramRegionsTitle.c_str());
        ImGui::PopFont();

        for (size_t i = 0; i < oe->heuristicData.regions.size(); ++i) {
            auto& region = oe->heuristicData.regions[i];

            ImGui::PushID(static_cast<int>(i));

            // Range controls
            ImGui::SetNextItemWidth(140.0f);
            int inputs[2] = { static_cast<int>(region.rect.X.Min), static_cast<int>(region.rect.X.Max) };
            if (ImGui::InputInt2("##Range", inputs)) {
                region.rect.X.Min = inputs[0];
                region.rect.X.Max = inputs[1];
            }

            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                oe->heuristicData.regions.erase(oe->heuristicData.regions.begin() + i);
                ImGui::PopID();
                break; // stop processing further to avoid invalid memory access
            }

            ImGui::SameLine();
            ImGui::ColorEdit4("##Color", (float*)&region.color,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::Button("Add Selection")) {
            HistogramRegion region;
            unsigned int minValue = oe->heuristicData.mainHistogram.minValue;
            unsigned int maxValue = oe->heuristicData.mainHistogram.maxValue;
            region.rect = ImPlotRect(minValue, maxValue, 0, 100);

            static const std::vector<ImVec4> defaultColors = {
                ImVec4(0.840f, 0.283f, 0.283f, 0.25f), // Red
                ImVec4(0.6f, 0.95f, 0.6f, 0.25f),      // Green
                ImVec4(0.95f, 0.95f, 0.6f, 0.25f),     // Yellow
                ImVec4(0.6f, 0.6f, 0.95f, 0.25f),      // Blue
                ImVec4(0.95f, 0.6f, 0.95f, 0.25f),     // Magenta
                ImVec4(0.6f, 0.95f, 0.95f, 0.25f),     // Cyan
                ImVec4(0.95f, 0.683f, 0.221f, 0.25f),  // Orange
            };
            static size_t nextColorIndex = 0;
            region.color = defaultColors[nextColorIndex % defaultColors.size()];
            nextColorIndex++;

            oe->heuristicData.regions.push_back(std::move(region));
        }
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

        // Render Main Histogram using ImPlot
        PrecomputedHistogram& hist = oe->heuristicData.mainHistogram;

        // Only draw if we actually have data (non-empty bins)
        bool hasData = std::any_of(hist.binCounts.begin(), hist.binCounts.end(),
                                [](int c){ return c > 0; });

        if (ImPlot::BeginPlot("##MainHistogramPlot", ImVec2(-1, -1))) {
            // Setup axes with auto-fit so they expand to your histogram range
            ImPlot::SetupAxes("Value", "Frequency", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

            PrecomputedHistogram& hist = oe->heuristicData.mainHistogram;
            bool hasData = std::any_of(hist.binCounts.begin(), hist.binCounts.end(),
                                    [](int c){ return c > 0; });

            if (hasData) {
                static std::vector<double> xs, ys;
                xs.resize(hist.binCount);
                ys.resize(hist.binCount);

                for (int i = 0; i < hist.binCount; i++) {
                    xs[i] = hist.minValue + (i + 0.5) * hist.binWidth;
                    ys[i] = static_cast<double>(hist.binCounts[i]);
                }

                ImPlot::PlotBars("Samples", xs.data(), ys.data(), hist.binCount, hist.binWidth);
            }

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
            if (m_onCommand) {
                m_onCommand(ProcessHistogramCommand{ m_uiState->selectedOEIndex });
            }
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
