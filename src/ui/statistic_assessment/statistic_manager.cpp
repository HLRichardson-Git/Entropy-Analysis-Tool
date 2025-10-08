
#include <algorithm>
#include <optional>

#include "statistic_manager.h"
#include "../../file_utils/file_utils.h"

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
            ImGui::BeginDisabled(oe->statisticData.nonIidSampleFilePath.empty() || oe->statisticData.nonIidTestRunning);
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Non-IID Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                if (m_onCommand) {
                    m_onCommand(RunNonIidTestCommand{
                        m_uiState->selectedOEIndex
                    });
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        if (oe->statisticData.nonIidTestRunning) {
            ImGui::SameLine();
            ImGui::PushFont(Config::normal);
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - oe->statisticData.nonIidStartTime).count();
            ImGui::Text("Running test %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
            ImGui::PopFont();
        }

        ImGui::Dummy(ImVec2(0.0f, 2 * ImGui::GetStyle().ItemSpacing.y));

        std::string resultText = (oe->statisticData.nonIidResult == "") ? "No Result Yet" : oe->statisticData.nonIidResult;

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

        //RenderUploadSectionForOE(oe);
        // create a unique id for this OE
        std::string idSuffix = std::to_string(reinterpret_cast<uintptr_t>(oe));
        std::string dlgId = std::string("StatisticRestartFileDlg_") + idSuffix;
        std::string buttonLabel = std::string("Load Raw Samples##") + idSuffix;

        // Call FileSelector with unique id / label:
        ImGui::PushFont(Config::normal);
        if (auto file = FileSelector(dlgId.c_str(), buttonLabel.c_str(), ".bin,.txt,.data,.*")) {
            fs::path destDir = fs::path(m_currentProject->path) / oe->oePath;
            // ensure destination dir exists
            std::error_code ec;
            fs::create_directories(destDir, ec);
            if (ec) {
                ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to create dest dir: %s", ec.message().c_str());
            } else {
                if (auto dest = CopyFileToDirectory(*file, destDir)) {
                    oe->statisticData.restartSampleFilePath = dest->string(); // success
                } else {
                    ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to copy file for %s", oe->oeName.c_str());
                }
            }
        }

        ImGui::SameLine();

        // display uploaded file path or placeholder
        if (!oe->statisticData.restartSampleFilePath.empty()) {
            fs::path filePath(oe->statisticData.restartSampleFilePath);
            std::string filename = filePath.filename().string();
            ImGui::TextWrapped("Current file: %s", filename.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(oe->statisticData.restartSampleFilePath.string().c_str());
                ImGui::EndTooltip();
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file loaded for this OE");
        }
        ImGui::PopFont();

        ImGui::PushFont(Config::normal);
        ImGui::Text("Input Min Entropy: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::InputDouble("##MinEntropy", &oe->statisticData.minEntropy);
        ImGui::PopFont();

        // Run Statistical Tests (main)
        ImGui::PushFont(Config::fontH3_Bold);
        ImVec2 statisticalTestSize(200, 30);

        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->statisticData.restartSampleFilePath.empty() || oe->statisticData.restartTestRunning);
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Restart Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                if (m_onCommand) {
                    m_onCommand(RunRestartTestCommand{
                        m_uiState->selectedOEIndex
                    });
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        if (oe->statisticData.restartTestRunning) {
            ImGui::SameLine();
            ImGui::PushFont(Config::normal);
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - oe->statisticData.restartStartTime).count();
            ImGui::Text("Running test %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
            ImGui::PopFont();
        }

        ImGui::Dummy(ImVec2(0.0f, 2 * ImGui::GetStyle().ItemSpacing.y));

        std::string resultText = (oe->statisticData.restartResult == "") ? "No Result Yet" : oe->statisticData.restartResult;

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
    if (auto file = FileSelector(dlgId.c_str(), buttonLabel.c_str(), ".bin,.txt,.data,.*")) {
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