

#include "statistic_manager.h"

#include <algorithm>

bool StatisticManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState) {
    m_dataManager = dataManager;
    m_config = config;
    SetCurrentProject(project);
    m_uiState = uiState;
    
    return true;
}

// Render
void StatisticManager::Render() {
    if (!m_currentProject) return;

    float fullWidth = ImGui::GetContentRegionAvail().x;
    float mainWidth = fullWidth;
    float cardHeight = 400.0f;

    auto oe = GetSelectedOE();
    if (!oe) return;

    ImGui::PushFont(Config::fontH2_Bold);
    ImGui::Text("Statistical Testing");
    ImGui::PopFont();
    ImGui::PushFont(Config::fontH3);
    ImGui::Text("Run NIST SP 800-90B test suites for %s", oe->oeName.c_str());
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 2 * ImGui::GetStyle().ItemSpacing.y));

    // NIST SP 800-90B Non-IID Test Suite Region
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("Non-IID Test Region", ImVec2(mainWidth, cardHeight), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        ImGui::Text("NIST SP 800-90B Non-IID Test Suite");
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 4 * ImGui::GetStyle().ItemSpacing.y));

        RenderUploadSectionForOE(oe);

        // Run Statistical Tests (main)
        ImGui::PushFont(Config::fontH3_Bold);
        ImVec2 statisticalTestSize(200, 30);

        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->statisticData.nonIidSampleFilePath.empty());
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Non-IID Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                if (oe->statisticData.nonIidSampleFilePath.empty()) return;

                /*oe->heuristicData.mainHistogram.testsRunning = true;
                oe->heuristicData.mainHistogram.startTime = std::chrono::steady_clock::now();

                if (m_onCommand) {
                    m_onCommand(RunStatisticalTestCommand{
                        m_uiState->selectedOEIndex,
                        0, // Main Histogram Index is `0`
                        oe->heuristicData.mainHistogram.heuristicFilePath.string(),
                        std::shared_ptr<lib90b::NonIidResult>(&oe->heuristicData.mainHistogram.entropyResults, [](lib90b::NonIidResult*){}),
                        std::nullopt,
                        std::nullopt
                    });
                }*/
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 2 * ImGui::GetStyle().ItemSpacing.y));

        // Summary/Full Output Button selector
        float padding = ImGui::GetStyle().ItemSpacing.x;
        float projectButtonWidth = (mainWidth - padding) * 0.5f;
        ImVec2 projectButtonSize(projectButtonWidth, 0);

        ImGui::PushFont(Config::fontH3);

        bool isSummaryActive = (nonIidTab == StatisticTabs::Summary);
        ImVec4 summaryColor = isSummaryActive ? Config::GREY_BUTTON.normal : Config::LIGHT_BACKGROUND_COLOR;
        ImVec4 summaryHoveredColor = isSummaryActive ? Config::GREY_BUTTON.normal : summaryColor;
        ImVec4 summaryFontColor = isSummaryActive ? Config::TEXT_DARK_CHARCOAL : Config::TEXT_LIGHT_GREY;

        // First button: Summary
        ImGui::PushStyleColor(ImGuiCol_Button,        summaryColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, summaryHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_Text, summaryFontColor);
        {
            if (ImGui::Button("Summary", projectButtonSize)) {
                nonIidTab = StatisticTabs::Summary;
            }
        }    
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, padding);

        // Second button: Full Output
        bool isFullOutputActive = (nonIidTab == StatisticTabs::FullOutput);
        ImVec4 fullOutColor = isFullOutputActive ? Config::GREY_BUTTON.normal : Config::LIGHT_BACKGROUND_COLOR;
        ImVec4 fullOutHoveredColor = isFullOutputActive ? Config::GREY_BUTTON.normal : fullOutColor;
        ImVec4 fullOutFontColor = isFullOutputActive ? Config::TEXT_DARK_CHARCOAL : Config::TEXT_LIGHT_GREY;

        ImGui::PushStyleColor(ImGuiCol_Button,        fullOutColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, fullOutHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_Text, fullOutFontColor);
        {
            if (ImGui::Button("Full Output", projectButtonSize)) {
                nonIidTab = StatisticTabs::FullOutput;
            }
        }    
        ImGui::PopStyleColor(4);

        ImGui::PopFont();

        std::string resultText = "";

        switch (nonIidTab) {
            case StatisticTabs::Summary:
                resultText = "Summary";

                break;
            case StatisticTabs::FullOutput:
                resultText = "Full Output";

                break;
        }

        ImGui::PushFont(Config::normal);
        ImGui::InputTextMultiline(
            "##readonly_text",                      // Label
            const_cast<char*>(resultText.c_str()),  // ImGui requires a non-const char*
            resultText.size() + 1,                  // Buffer size
            ImGui::GetContentRegionAvail(),         // Size (Takes remaining height of child)
            ImGuiInputTextFlags_ReadOnly            // Make it read-only
        );
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // NIST SP 800-90B Restart Test Suite Region
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("Restart Test Region", ImVec2(mainWidth, cardHeight), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        ImGui::Text("NIST SP 800-90B Restart Test Suite");
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 4 * ImGui::GetStyle().ItemSpacing.y));

        RenderUploadSectionForOE(oe);

        // Run Statistical Tests (main)
        ImGui::PushFont(Config::fontH3_Bold);
        ImVec2 statisticalTestSize(200, 30);

        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->statisticData.restartSampleFilePath.empty());
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Restart Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {

                /*oe->heuristicData.mainHistogram.testsRunning = true;
                oe->heuristicData.mainHistogram.startTime = std::chrono::steady_clock::now();

                if (m_onCommand) {
                    m_onCommand(RunStatisticalTestCommand{
                        m_uiState->selectedOEIndex,
                        0, // Main Histogram Index is `0`
                        oe->heuristicData.mainHistogram.heuristicFilePath.string(),
                        std::shared_ptr<lib90b::NonIidResult>(&oe->heuristicData.mainHistogram.entropyResults, [](lib90b::NonIidResult*){}),
                        std::nullopt,
                        std::nullopt
                    });
                }*/
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 2 * ImGui::GetStyle().ItemSpacing.y));

        // Summary/Full Output Button selector
        float padding = ImGui::GetStyle().ItemSpacing.x;
        float projectButtonWidth = (mainWidth - padding) * 0.5f;
        ImVec2 projectButtonSize(projectButtonWidth, 0);

        ImGui::PushFont(Config::fontH3);

        bool isSummaryActive = (restartTab == StatisticTabs::Summary);
        ImVec4 summaryColor = isSummaryActive ? Config::GREY_BUTTON.normal : Config::LIGHT_BACKGROUND_COLOR;
        ImVec4 summaryHoveredColor = isSummaryActive ? Config::GREY_BUTTON.normal : summaryColor;
        ImVec4 summaryFontColor = isSummaryActive ? Config::TEXT_DARK_CHARCOAL : Config::TEXT_LIGHT_GREY;

        // First button: Summary
        ImGui::PushStyleColor(ImGuiCol_Button,        summaryColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, summaryHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_Text, summaryFontColor);
        {
            if (ImGui::Button("Summary", projectButtonSize)) {
                restartTab = StatisticTabs::Summary;
            }
        }    
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, padding);

        // Second button: Full Output
        bool isFullOutputActive = (restartTab == StatisticTabs::FullOutput);
        ImVec4 fullOutColor = isFullOutputActive ? Config::GREY_BUTTON.normal : Config::LIGHT_BACKGROUND_COLOR;
        ImVec4 fullOutHoveredColor = isFullOutputActive ? Config::GREY_BUTTON.normal : fullOutColor;
        ImVec4 fullOutFontColor = isFullOutputActive ? Config::TEXT_DARK_CHARCOAL : Config::TEXT_LIGHT_GREY;

        ImGui::PushStyleColor(ImGuiCol_Button,        fullOutColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, fullOutHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_Text, fullOutFontColor);
        {
            if (ImGui::Button("Full Output", projectButtonSize)) {
                restartTab = StatisticTabs::FullOutput;
            }
        }    
        ImGui::PopStyleColor(4);

        ImGui::PopFont();

        std::string resultText = "";

        switch (restartTab) {
            case StatisticTabs::Summary:
                resultText = "Summary";

                break;
            case StatisticTabs::FullOutput:
                resultText = "Full Output";

                break;
        }

        ImGui::PushFont(Config::normal);
        ImGui::InputTextMultiline(
            "##readonly_text",                      // Label
            const_cast<char*>(resultText.c_str()),  // ImGui requires a non-const char*
            resultText.size() + 1,                  // Buffer size
            ImGui::GetContentRegionAvail(),         // Size (Takes remaining height of child)
            ImGuiInputTextFlags_ReadOnly            // Make it read-only
        );
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    RenderPopups();
}

OperationalEnvironment* StatisticManager::GetSelectedOE() {
    if (!m_currentProject || !m_uiState) return nullptr;
    int idx = m_uiState->selectedOEIndex;
    if (idx < 0 || idx >= (int)m_currentProject->operationalEnvironments.size())
        return nullptr;
    return &m_currentProject->operationalEnvironments[idx];
}

void StatisticManager::RenderPopups() { 
    //if (m_editHistogramPopupOpen) {
    //    RenderMainHistogramConfigPopup();
    //}
}

void StatisticManager::RenderUploadSectionForOE(OperationalEnvironment* oe) {
    if (!oe) return;

    // create a unique id for this OE
    std::string idSuffix = std::to_string(reinterpret_cast<uintptr_t>(oe));
    std::string dlgId = std::string("StatisticFileDlg_") + idSuffix;
    std::string buttonLabel = std::string("Load Raw Samples##") + idSuffix;

    // Call FileSelector with unique id / label:
    ImGui::PushFont(Config::normal);
    if (auto file = FileSelector(dlgId.c_str(), buttonLabel.c_str(), ".data,.bin,.txt,.*")) {
        fs::path destDir = fs::path(m_currentProject->path) / oe->oePath;
        // ensure destination dir exists
        std::error_code ec;
        fs::create_directories(destDir, ec);
        if (ec) {
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to create dest dir: %s", ec.message().c_str());
        } else {
            if (auto dest = CopyFileToDirectory(*file, destDir)) {
                oe->statisticData.nonIidSampleFilePath = dest->string(); // success
            } else {
                ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to copy file for %s", oe->oeName.c_str());
            }
        }
    }

    ImGui::SameLine();

    // display uploaded file path or placeholder
    if (!oe->statisticData.nonIidSampleFilePath.empty()) {
        fs::path filePath(oe->statisticData.nonIidSampleFilePath);
        std::string filename = filePath.filename().string();
        ImGui::TextWrapped("Current file: %s", filename.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(oe->statisticData.nonIidSampleFilePath.string().c_str());
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file loaded for this OE");
    }
    ImGui::PopFont();
}