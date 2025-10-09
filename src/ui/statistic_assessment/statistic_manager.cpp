
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

        RenderUploadSectionForOE(oe, oe->statisticData.nonIidSampleFilePath, "Load Samples for Non-IID", ".bin,.txt,.data,.*");

        // Run NIST SP 800-90B Non-IID Tests
        ImGui::PushFont(Config::fontH3_Bold);
        ImVec2 statisticalTestSize(200, 30);

        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->statisticData.nonIidSampleFilePath.empty() || oe->statisticData.nonIidTestTimer.testRunning);
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Non-IID Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                if (m_onCommand) {
                    m_onCommand(RunNonIidTestCommand{
                        oe->statisticData.nonIidSampleFilePath,
                        &oe->statisticData.nonIidResultFilePath,
                        &oe->statisticData.nonIidResult,
                        &oe->statisticData.nonIidParsedResults,
                        &oe->statisticData.nonIidTestTimer
                    });
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        if (oe->statisticData.nonIidTestTimer.testRunning) {
            ImGui::SameLine();
            ImGui::PushFont(Config::normal);
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - oe->statisticData.nonIidTestTimer.testStartTime).count();
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

        RenderUploadSectionForOE(oe, oe->statisticData.restartSampleFilePath, "Load Samples for Restart", ".bin,.txt,.data,.*");

        ImGui::PushFont(Config::normal);
        ImGui::Text("Input Min Entropy: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::InputDouble("##MinEntropy", &oe->statisticData.nonIidParsedResults.minEntropy);
        ImGui::PopFont();

        // Run NIST SP 800-90B Restart Tests
        ImGui::PushFont(Config::fontH3_Bold);
        ImVec2 statisticalTestSize(200, 30);

        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->statisticData.restartSampleFilePath.empty() || oe->statisticData.restartTestTimer.testRunning);
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Restart Test Suite";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                if (m_onCommand) {
                    m_onCommand(RunRestartTestCommand{
                        oe->statisticData.nonIidParsedResults.minEntropy,
                        oe->statisticData.restartSampleFilePath,
                        &oe->statisticData.restartResultFilePath,
                        &oe->statisticData.restartResult,
                        &oe->statisticData.restartTestTimer
                    });
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        if (oe->statisticData.restartTestTimer.testRunning) {
            ImGui::SameLine();
            ImGui::PushFont(Config::normal);
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - oe->statisticData.restartTestTimer.testStartTime).count();
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

void StatisticManager::RenderUploadSectionForOE(
    OperationalEnvironment* oe,
    std::filesystem::path& filePathVar,
    const std::string& labelPrefix,
    const std::string& fileExtensions,
    const ImVec2& buttonSize,
    const Config::ButtonPalette& buttonColor,
    const ImVec4& textColor
) {
    if (!oe || !m_currentProject) return;

    ImGui::PushID(&filePathVar);
    ImGui::PushFont(Config::normal);

    std::string idSuffix = std::to_string(reinterpret_cast<uintptr_t>(oe)) + "_" + labelPrefix;
    std::string dlgId = "StatisticFileDlg_" + idSuffix;
    std::string buttonLabel = labelPrefix + "##" + idSuffix;

    if (auto file = FileSelector(dlgId.c_str(), buttonLabel.c_str(), fileExtensions.c_str(), ".", buttonSize)) {
        fs::path destDir = fs::path(m_currentProject->path) / oe->oePath;
        std::error_code ec;
        fs::create_directories(destDir, ec);
        if (ec) {
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to create dest dir: %s", ec.message().c_str());
        } else {
            if (auto dest = CopyFileToDirectory(*file, destDir)) {
                filePathVar = dest->string();
            } else {
                ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to copy file for %s", oe->oeName.c_str());
            }
        }
    }

    ImGui::SameLine();

    // Current file display
    if (!filePathVar.empty()) {
        fs::path filePath(filePathVar);
        std::string filename = filePath.filename().string();

        ImGui::TextWrapped("Current file: %s", filename.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(filePathVar.string().c_str());
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No %s loaded for this OE", labelPrefix.c_str());
    }

    ImGui::PopFont();
    ImGui::PopID();
}


void StatisticManager::RenderPopups() { 
    if (m_showBatchPopup) {
        RenderBatchStatistic();
    }
}

void StatisticManager::RenderBatchStatistic() {
    if (m_showBatchPopup)
        ImGui::OpenPopup("Batch Statistic Analysis");

    ImVec2 minSize = ImVec2(800, 630);
    ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);

    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
    if (ImGui::BeginPopupModal("Batch Statistic Analysis", NULL)) {
        static int currentStep = 0; // 0 = upload, 1 = non-iid, 2 = restart
        static bool stepCompleted[3] = { false, false, false };

        ImGui::PushFont(Config::fontH2_Bold);
        ImGui::Text("Batch Statistic Analysis");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushFont(Config::fontH3);

        // Step indicators
        const char* stepNames[3] = { "1. Upload Samples", "2. Run Non-IID Tests", "3. Run Restart Tests" };
        ImGui::Text("Progress:");
        for (int i = 0; i < 3; ++i) {
            if (i == currentStep)
                ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_PURPLE);
            else if (stepCompleted[i])
                ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_GREEN);
            else
                ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);

            ImGui::BulletText("%s", stepNames[i]);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Step 1: Upload files
        if (currentStep == 0) {
            ImGui::Text("Step 1: Upload binary files for each OE.");
            ImGui::Separator();
            ImGui::Spacing();

            for (auto& oe : m_currentProject->operationalEnvironments) {
                ImGui::PushID(oe.oeName.c_str());
                ImGui::Text("%s", oe.oeName.c_str());

                // Upload Non-IID file
                ImGui::TextColored(Config::TEXT_PURPLE, "Non-IID Sample File:");
                ImGui::SameLine();
                RenderUploadSectionForOE(&oe, oe.statisticData.nonIidSampleFilePath, "nonIid_batch", ".bin,.txt,.data,.*");

                // Upload Restart file
                ImGui::TextColored(Config::TEXT_BLUE, "Restart Sample File:");
                ImGui::SameLine();
                RenderUploadSectionForOE(&oe, oe.statisticData.restartSampleFilePath, "restart_batch", ".bin,.txt,.data,.*");

                ImGui::PopID();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // Automatically validate uploads before allowing "Next"
            bool allUploaded = std::all_of(
                m_currentProject->operationalEnvironments.begin(),
                m_currentProject->operationalEnvironments.end(),
                [](const auto& oe) {
                    return !oe.statisticData.nonIidSampleFilePath.empty() &&
                           !oe.statisticData.restartSampleFilePath.empty();
                }
            );

            stepCompleted[0] = allUploaded;

            if (!allUploaded && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Please upload files for all OEs before proceeding.");
            }

            // Missing file popup (only shown if user tries to skip manually)
            if (ImGui::BeginPopupModal("Missing Files", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Please upload files for all Operational Environments before continuing.");
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        // Step 2: Run Non-IID tests
        else if (currentStep == 1) {
            ImGui::Text("Step 2: Run Non-IID tests.");
            ImGui::TextWrapped("This will run the NIST SP 800-90B Non-IID test suite on the samples.");

            if (ImGui::Button("Run Non-IID tests", ImVec2(200, 0))) {
                if (m_onCommand) {
                    // Loop through all Operational Environments
                    for (auto& oe : m_currentProject->operationalEnvironments) {
                        m_onCommand(RunNonIidTestCommand{
                            oe.statisticData.nonIidSampleFilePath,
                            &oe.statisticData.nonIidResultFilePath,
                            &oe.statisticData.nonIidResult,
                            &oe.statisticData.nonIidParsedResults,
                            &oe.statisticData.nonIidTestTimer
                        });
                    }
                }
            }

            stepCompleted[1] = true; // Allow user to skip this test, and go straight to restart
        }

        // Step 3: Run Restart tests
        else if (currentStep == 2) {
            ImGui::Text("Step 3: Run Restart tests.");
            ImGui::TextWrapped("This will run the NIST SP 800-90B Restart test suite on the samples.");

            if (ImGui::Button("Run Restart tests", ImVec2(200, 0))) {
                if (m_onCommand) {
                    // Loop through all Operational Environments
                    for (auto& oe : m_currentProject->operationalEnvironments) {
                        m_onCommand(RunRestartTestCommand{
                            oe.statisticData.nonIidParsedResults.minEntropy,
                            oe.statisticData.restartSampleFilePath,
                            &oe.statisticData.restartResultFilePath,
                            &oe.statisticData.restartResult,
                            &oe.statisticData.restartTestTimer
                        });
                    }
                }
            }

            stepCompleted[2] = true; 
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Navigation buttons
        ImGui::BeginDisabled(currentStep == 0);
        std::string backButton = std::string(reinterpret_cast<const char*>(u8"\uf060")) + "  Back";
        if (ImGui::Button(backButton.c_str(), ImVec2(100, 0))) {
            if (currentStep > 0) currentStep--;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!stepCompleted[currentStep] || currentStep >= 2);
        std::string nextButton = std::string(reinterpret_cast<const char*>(u8"\uf061")) + "  Next";
        if (ImGui::Button(nextButton.c_str(), ImVec2(100, 0))) {
            if (currentStep < 2) currentStep++;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Close", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            m_showBatchPopup = false;
            currentStep = 0;
            memset(stepCompleted, 0, sizeof(stepCompleted));
        }

        ImGui::PopFont();

        ImGui::EndPopup();
    }
}