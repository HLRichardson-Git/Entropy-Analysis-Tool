
#include "heuristic_manager.h"

#include <algorithm>

#include <lib90b/non_iid.h>

bool HeuristicManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project, UIState* uiState) {
    m_dataManager = dataManager;
    m_config = config;
    SetCurrentProject(project);
    m_uiState = uiState;
    
    return true;
}

// -----------------------------------------------------------------------------
// Render - full refactor using MainHistogram::subHists (SubHistogram)
// -----------------------------------------------------------------------------
void HeuristicManager::Render() {
    if (!m_currentProject) return;

    float fullWidth = ImGui::GetContentRegionAvail().x;
    float sidebarWidth = 220.0f;
    float mainWidth = fullWidth - sidebarWidth;

    auto oe = GetSelectedOE();
    if (!oe) return;

    // ---------------------------------------------------------------------
    // Sidebar: Selected sub-histograms (was regions)
    // ---------------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("SelectMainHistogramRegions", ImVec2(sidebarWidth, 460), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string selectMainHistogramRegionsTitle = std::string(reinterpret_cast<const char*>(u8"\uf0fe")) + "  Selected Regions";
        ImGui::Text(selectMainHistogramRegionsTitle.c_str());
        ImGui::PopFont();

        ImGui::PushFont(Config::normal);

        auto& mainHist = oe->heuristicData.mainHistogram;
        auto& subHists = mainHist.subHists;

        for (size_t i = 0; i < subHists.size(); ++i) {
            auto& sub = subHists[i];
            ImGui::PushID(static_cast<int>(i));

            // Range controls
            ImGui::SetNextItemWidth(140.0f);
            int inputs[2] = { static_cast<int>(sub.rect.X.Min), static_cast<int>(sub.rect.X.Max) };
            if (ImGui::InputInt2("##Range", inputs)) {
                sub.rect.X.Min = inputs[0];
                sub.rect.X.Max = inputs[1];
            }

            ImGui::SameLine();
            std::string deleteButton = std::string(reinterpret_cast<const char*>(u8"\uf1f8"));
            if (ImGui::Button(deleteButton.c_str())) {
                subHists.erase(subHists.begin() + i);
                ImGui::PopID();
                break; // avoid invalid access after erase
            }

            ImGui::SameLine();
            ImGui::ColorEdit4("##Color", (float*)&sub.color,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::PopID();
        }

        ImGui::Separator();

        // Disable "Add Selection" if histogram has no data
        bool hasHistogram = std::any_of(
            mainHist.binCounts.begin(), mainHist.binCounts.end(),
            [](int c){ return c > 0; }
        );

        ImGui::BeginDisabled(!hasHistogram);
        if (ImGui::Button("Add Selection")) {
            SubHistogram sub;
            unsigned int minValue = mainHist.minValue;
            unsigned int maxValue = mainHist.maxValue;
            sub.rect = ImPlotRect(minValue, maxValue, 0, 100);

            // Compute initial sub-histogram metadata from rect
            sub.minValue = static_cast<unsigned int>(sub.rect.X.Min);
            sub.maxValue = static_cast<unsigned int>(sub.rect.X.Max);

            int subBinCount = std::max(1, static_cast<int>((sub.maxValue - sub.minValue) / mainHist.binWidth));
            sub.binWidth = static_cast<double>(sub.maxValue - sub.minValue) / subBinCount;

            // Color & index
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
            sub.color = defaultColors[nextColorIndex % defaultColors.size()];
            nextColorIndex++;
            sub.subHistIndex = static_cast<int>(subHists.size()) + 1;

            mainHist.subHists.push_back(std::move(sub));
        }
        ImGui::EndDisabled();

        ImGui::PopFont();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // ---------------------------------------------------------------------
    // Main histogram area + top-right buttons
    // ---------------------------------------------------------------------
    ImGui::BeginChild("MainHistogram", ImVec2(mainWidth - 10.0f, 460), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string mainHeuristicTitle = std::string(reinterpret_cast<const char*>(u8"\uf1fe")) + "  Main Histogram - " + oe->oeName;
        ImGui::Text(mainHeuristicTitle.c_str());

        // Buttons group on right
        ImVec2 statisticalTestSize(200, 30);
        ImVec2 findFirstDecimationSize(220, 30);
        ImVec2 cogSize(30, 30);
        float spacing = 8.0f;
        float totalWidth = statisticalTestSize.x + spacing + findFirstDecimationSize.x + spacing + cogSize.x;
        float regionWidth = ImGui::GetContentRegionAvail().x;
        float startX = regionWidth - totalWidth + 5.0f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(startX);

        ImGui::PushFont(Config::fontH3_Bold);

        // Run Statistical Tests (main)
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->heuristicData.mainHistogram.convertedFilePath.empty() || oe->heuristicData.mainHistogram.testsRunning);
            std::string runStatisticalTestButton = std::string(reinterpret_cast<const char*>(u8"\uf83e")) + "  Run Statistical Tests";
            if (ImGui::Button(runStatisticalTestButton.c_str(), statisticalTestSize)) {
                oe->heuristicData.mainHistogram.testsRunning = true;
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
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, spacing);

        // Find Passing Decimation
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::RED_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::RED_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::RED_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
        {
            ImGui::BeginDisabled(oe->heuristicData.mainHistogram.convertedFilePath.empty() || oe->heuristicData.mainHistogram.decimationRunning);
            std::string findFirstPassingDecimationButton = std::string(reinterpret_cast<const char*>(u8"\uf12d")) + "  Find Passing Decimation";
            if (ImGui::Button(findFirstPassingDecimationButton.c_str(), findFirstDecimationSize)) {
                const fs::path convertedFilePath = oe->heuristicData.mainHistogram.convertedFilePath;

                if (m_onCommand) {
                    m_onCommand(FindPassingDecimationCommand{
                        m_uiState->selectedOEIndex,
                        convertedFilePath,
                        std::make_shared<std::string>()  // output will be set in the thread
                    });
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, spacing);

        // Cog button to edit histogram
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::WHITE_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::WHITE_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::WHITE_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
        {
            std::string cogButton = std::string(reinterpret_cast<const char*>(u8"\uf013"));
            if (ImGui::Button(cogButton.c_str(), cogSize)) {
                m_editHistogramPopupOpen = true;
            }
            ImGui::PopFont();
        }
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        // Render main histogram (bars)
        auto& hist = oe->heuristicData.mainHistogram;
        bool hasData = std::any_of(hist.binCounts.begin(), hist.binCounts.end(), [](int c){ return c > 0; });

        std::string plotLabel = "##MainHistogramPlot_" + oe->oeName;
        if (ImPlot::BeginPlot(plotLabel.c_str(), ImVec2(-1, 300))) {
            ImPlot::SetupAxes("Value", "Frequency", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

            if (hasData) {
                static std::vector<double> xs, ys;
                xs.resize(hist.binCount);
                ys.resize(hist.binCount);

                for (int i = 0; i < hist.binCount; ++i) {
                    xs[i] = hist.minValue + (i + 0.5) * hist.binWidth;
                    ys[i] = static_cast<double>(hist.binCounts[i]);
                }

                ImPlot::PlotBars("##MainHistogramSamples", xs.data(), ys.data(), hist.binCount, hist.binWidth);
            }

            // Draw drag rects for sub-hists
            for (size_t i = 0; i < hist.subHists.size(); ++i) {
                auto& sub = hist.subHists[i];
                ImPlotRect& rect = sub.rect;

                ImPlotRect plotLimits = ImPlot::GetPlotLimits();
                rect.Y.Min = plotLimits.Y.Min;
                rect.Y.Max = plotLimits.Y.Max;

                ImPlot::DragRect(static_cast<int>(i), 
                                 &rect.X.Min, &rect.Y.Min,
                                 &rect.X.Max, &rect.Y.Max,
                                 sub.color,
                                 ImPlotDragToolFlags_None);

                // Clamp X to histogram min/max
                rect.X.Min = std::max(rect.X.Min, static_cast<double>(hist.minValue));
                rect.X.Max = std::min(rect.X.Max, static_cast<double>(hist.maxValue));

                if (rect.X.Min > rect.X.Max) std::swap(rect.X.Min, rect.X.Max);
            }

            ImPlot::EndPlot();
        }

        // Metadata Area
        ImGui::PushFont(Config::normal);
        ImGui::Separator();

        // Main Non-IID results
        ImGui::Text("Main Histogram Non-IID results:");
        if (hist.entropyResults.min_entropy.has_value()) {
            const auto& res = hist.entropyResults;

            if (res.H_original.has_value()) {
                ImGui::TextDisabled("(H_original)");
                ImGui::SameLine();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.6f", res.H_original.value());
                ImVec2 textSize = ImGui::CalcTextSize(buf);
                if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
                ImGui::SameLine();
            }

            if (res.H_bitstring.has_value()) {
                ImGui::TextDisabled("(H_bitstring)");
                ImGui::SameLine();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.6f", res.H_bitstring.value());
                ImVec2 textSize = ImGui::CalcTextSize(buf);
                if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
                ImGui::SameLine();
            }

            {
                ImGui::TextDisabled("(Min Entropy)");
                ImGui::SameLine();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.6f", res.min_entropy.value());
                ImVec2 textSize = ImGui::CalcTextSize(buf);
                if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
            }

        } else if (hist.testsRunning) {
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - hist.startTime).count();
            ImGui::BulletText("Running tests %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
        } else {
            ImGui::BulletText("Non-IID results not available");
        }

        ImGui::Separator();

        ImGui::Text("First Passing Decimation result:");
        if (!hist.firstPassingDecimationResult.empty()) {
            ImGui::BulletText("%s", hist.firstPassingDecimationResult.c_str());
        } else if (hist.decimationRunning) {
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - hist.decimationStartTime).count();
            ImGui::BulletText("Running tests %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
        } else {
            ImGui::BulletText("First Passing Decimation results not available");
        }

        ImGui::PopFont();
    }
    ImGui::EndChild();

    // ---------------------------------------------------------------------
    // Sub-histograms: iterate mainHistogram.subHists
    // ---------------------------------------------------------------------
    ImGui::BeginChild("SubHistograms", ImVec2(fullWidth, 0), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string subHistogramsTitle = std::string(reinterpret_cast<const char*>(u8"\uf1fe")) + "  Sub Histograms";
        ImGui::Text(subHistogramsTitle.c_str());
        ImGui::PopFont();
        ImGui::Separator();

        auto& main = oe->heuristicData.mainHistogram;
        for (size_t i = 0; i < main.subHists.size(); ++i) {
            auto& sub = main.subHists[i];

            // Convert rect range to bin indexes
            int binMin = static_cast<int>((sub.rect.X.Min - main.minValue) / main.binWidth);
            int binMax = static_cast<int>((sub.rect.X.Max - main.minValue) / main.binWidth);
            binMin = std::clamp(binMin, 0, main.binCount - 1);
            binMax = std::clamp(binMax, 0, main.binCount - 1);
            if (binMin > binMax) std::swap(binMin, binMax);

            // Prepare sub-histogram plot data (from main bins)
            std::vector<double> xs, ys;
            xs.reserve(binMax - binMin + 1);
            ys.reserve(binMax - binMin + 1);
            for (int b = binMin; b <= binMax; ++b) {
                double xCenter = main.minValue + (b + 0.5) * main.binWidth;
                xs.push_back(xCenter);
                ys.push_back(static_cast<double>(main.binCounts[b]));
            }

            // Each sub-histogram child
            std::string childLabel = "SubHistogramChild_" + std::to_string(i);
            ImGui::BeginChild(childLabel.c_str(), ImVec2(-1, 260), true);
            {
                // Left column: metadata + buttons
                ImGui::BeginChild(("SubMeta_" + std::to_string(i)).c_str(), ImVec2(sidebarWidth - 10.0f, -1), false);
                {
                    std::string regionTitle = "Region " + std::to_string(sub.subHistIndex);

                    ImGui::PushFont(Config::fontH3_Bold);
                    ImGui::Text(regionTitle.c_str());
                    ImGui::PopFont();

                    // Buttons aligned right
                    ImVec2 runTestSize(30, 30);
                    ImVec2 configSize(30, 30);
                    float spacing = 6.0f;
                    float totalWidth = runTestSize.x + spacing + configSize.x;
                    float regionWidth = ImGui::GetContentRegionAvail().x;
                    float startX = regionWidth - totalWidth + ImGui::GetStyle().ItemSpacing.x;
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(startX - 16.0f);

                    ImGui::PushFont(Config::normal);

                    // Run Test (for this sub-histogram)
                    ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
                    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_LIGHT_GREY);
                    {
                        if (ImGui::Button(std::string(reinterpret_cast<const char*>(u8"\uf83e")).c_str())) {
                            if (m_onCommand) {
                                m_onCommand(RunStatisticalTestCommand{
                                    m_uiState->selectedOEIndex,
                                    sub.subHistIndex,
                                    oe->heuristicData.mainHistogram.heuristicFilePath.string(),
                                    std::shared_ptr<lib90b::NonIidResult>(&sub.entropyResults, [](lib90b::NonIidResult*){}),
                                    sub.rect.X.Min,
                                    sub.rect.X.Max
                                });
                            }
                        }
                    }
                    ImGui::PopStyleColor(4);

                    ImGui::SameLine(0, spacing);

                    // Config (cog)
                    ImGui::PushStyleColor(ImGuiCol_Button,        Config::WHITE_BUTTON.normal);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::WHITE_BUTTON.hovered);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::WHITE_BUTTON.active);
                    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
                    {
                        if (ImGui::Button((std::string(reinterpret_cast<const char*>(u8"\uf013")).c_str()))) {
                            // TODO: open sub-histogram config
                        }
                    }
                    ImGui::PopStyleColor(4);

                    // Metadata below
                    ImGui::Separator();
                    ImGui::Text("%s Metadata:", regionTitle.c_str());
                    ImGui::BulletText("Min: %u", sub.minValue);
                    ImGui::BulletText("Max: %u", sub.maxValue);

                    int subBinCount = std::max(0, binMax - binMin + 1);
                    ImGui::BulletText("Bins: %d", subBinCount);
                    ImGui::BulletText("Bin Width: %.2f", main.binWidth);

                    ImGui::Separator();
                    ImGui::Text("%s Non-IID results:", regionTitle.c_str());

                    if (sub.entropyResults.min_entropy.has_value()) {
                        const auto& res = sub.entropyResults;
                        
                        if (res.H_original.has_value()) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            ImGui::Text("H_original:");
                            ImGui::SameLine();
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%.6f", res.H_original.value());
                            ImVec2 textSize = ImGui::CalcTextSize(buf);
                            if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
                            ImGui::SameLine();
                            ImGui::Text("bits");
                        }
                        
                        if (res.H_bitstring.has_value()) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            ImGui::Text("H_bitstring:");
                            ImGui::SameLine();
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%.6f", res.H_bitstring.value());
                            ImVec2 textSize = ImGui::CalcTextSize(buf);
                            if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
                            ImGui::SameLine();
                            ImGui::Text("bits");
                        }
                        
                        {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            ImGui::Text("Min Entropy:");
                            ImGui::SameLine();
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%.6f", res.min_entropy.value());
                            ImVec2 textSize = ImGui::CalcTextSize(buf);
                            if (ImGui::Selectable(buf, false, 0, textSize)) ImGui::SetClipboardText(buf);
                            ImGui::SameLine();
                            ImGui::Text("bits");
                        }
                    } else if (sub.testsRunning) {
                        float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - sub.startTime).count();
                        ImGui::BulletText("Running %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
                    } else {
                        ImGui::BulletText("Non-IID results not available");
                    }

                    ImGui::PopFont();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                // Right column (plot)
                ImGui::BeginChild(("SubPlot_" + std::to_string(i)).c_str(), ImVec2(-1, -1), false);
                {
                    std::string plotLabel = "##SubHistogramPlot_" + std::to_string(i);
                    if (ImPlot::BeginPlot(plotLabel.c_str(), ImVec2(-1, -1))) {
                        ImPlot::SetupAxes("Value", "Frequency", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                        if (!xs.empty()) {
                            ImVec4 opaqueColor = sub.color;
                            opaqueColor.w = 1.0f;
                            ImPlot::SetNextFillStyle(opaqueColor);
                            ImPlot::PlotBars("##SubHistogramSamples", xs.data(), ys.data(), (int)xs.size(), main.binWidth);
                        }
                        ImPlot::EndPlot();
                    }
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();

            ImGui::Spacing();
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
    } else if (m_showBatchPopup) {
        RenderBatchHeuristic();
    }
}

void HeuristicManager::RenderUploadSectionForOE(OperationalEnvironment* oe)
{
    if (!oe) return;

    // create a unique id for this OE (pointer-based)
    std::string idSuffix = std::to_string(reinterpret_cast<uintptr_t>(oe)); // stable for this run
    std::string dlgId = std::string("HistogramFileDlg_") + idSuffix;
    std::string buttonLabel = std::string("Load Raw Samples##") + idSuffix; // '##' hides uniqueness from UI

    // Call FileSelector with unique id / label:
    // Adjust these args to your FileSelector signature. In your earlier code you used:
    // FileSelector("HistogramFileDlg", "Load Raw Samples", ".data,.bin,.txt,.*")
    // So pass dialog id and a unique label:
    if (auto file = FileSelector(dlgId.c_str(), buttonLabel.c_str(), ".data,.bin,.txt,.*"))
    {
        fs::path destDir = fs::path(m_currentProject->path) / oe->oePath;
        // ensure destination dir exists
        std::error_code ec;
        fs::create_directories(destDir, ec);
        if (ec) {
            // optional: show error
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to create dest dir: %s", ec.message().c_str());
        } else {
            if (auto dest = CopyFileToDirectory(*file, destDir)) {
                oe->heuristicData.mainHistogram.heuristicFilePath = dest->string(); // success
            } else {
                ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Failed to copy file for %s", oe->oeName.c_str());
            }
        }
    }

    ImGui::SameLine();

    // display uploaded file path or placeholder
    if (!oe->heuristicData.mainHistogram.heuristicFilePath.empty()) {
        fs::path filePath(oe->heuristicData.mainHistogram.heuristicFilePath);
        std::string filename = filePath.filename().string();
        ImGui::TextWrapped("Current file: %s", filename.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(oe->heuristicData.mainHistogram.heuristicFilePath.string().c_str());
            ImGui::EndTooltip();
        }
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file loaded for this OE");
    }
}

void HeuristicManager::RenderMainHistogramConfigPopup() {
    if (m_editHistogramPopupOpen) {
        ImGui::OpenPopup("Edit Main Histogram");
    }

    ImVec2 minSize(400, 200);
    ImVec2 maxSize(FLT_MAX, FLT_MAX);
    ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

    if (ImGui::BeginPopupModal("Edit Main Histogram", nullptr)) {
        auto oe = GetSelectedOE();
        if (!oe) { ImGui::EndPopup(); return; }

        std::string editMainHeuristicTitle = "Edit Main Histogram - " + oe->oeName;
        ImGui::Text(editMainHeuristicTitle.c_str());

        // File selection — store in mainHistogram
        RenderUploadSectionForOE(oe);

        if (ImGui::Button("Process uploaded file")) {
            if (m_onCommand) {
                m_onCommand(ProcessHistogramCommand{ m_uiState->selectedOEIndex });
            }
        }

        ImGui::PushFont(Config::normal);
        ImGui::Separator();
        ImGui::Text("Main Histogram Metadata:");
        ImGui::BulletText("Bins: %d", oe->heuristicData.mainHistogram.binCount);
        ImGui::SameLine();
        ImGui::BulletText("Min: %d", oe->heuristicData.mainHistogram.minValue);
        ImGui::SameLine();
        ImGui::BulletText("Max: %d", oe->heuristicData.mainHistogram.maxValue);
        ImGui::SameLine();
        ImGui::BulletText("Bin Width: %.2f", oe->heuristicData.mainHistogram.binWidth);
        ImGui::PopFont();
        ImGui::Separator();

        // Cancel button at bottom
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 buttonSize = ImVec2(120, 0);
        float bottomY = windowSize.y - ImGui::GetStyle().WindowPadding.y - buttonSize.y - 20.0f;
        ImGui::SetCursorPosY(bottomY);
        if (ImGui::Button("Cancel", buttonSize)) {
            ImGui::CloseCurrentPopup();
            m_editHistogramPopupOpen = false;
        }

        ImGui::EndPopup();
    }
}

void HeuristicManager::RenderBatchHeuristic() {
    if (m_showBatchPopup)
        ImGui::OpenPopup("Batch Heuristic Analysis");

    if (ImGui::BeginPopupModal("Batch Heuristic Analysis", NULL)) {
        static int currentStep = 0; // 0 = upload, 1 = process, 2 = run
        static bool stepCompleted[3] = { false, false, false };

        ImGui::PushFont(Config::fontH2_Bold);
        ImGui::Text("Batch Heuristic Analysis");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushFont(Config::fontH3);

        // Step indicators
        const char* stepNames[3] = { "1. Upload Samples", "2. Convert to Histograms", "3. Run Statistical Tests" };
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
            ImGui::Text("Step 1: Upload raw sample files for each OE.");
            ImGui::Separator();
            ImGui::Spacing();

            for (auto& oe : m_currentProject->operationalEnvironments) {
                ImGui::PushID(oe.oeName.c_str());
                ImGui::Text("%s", oe.oeName.c_str());
                ImGui::SameLine();
                RenderUploadSectionForOE(&oe);
                ImGui::PopID();
                ImGui::Separator();
            }

            // Automatically validate uploads before allowing "Next"
            bool allUploaded = std::all_of(
                m_currentProject->operationalEnvironments.begin(),
                m_currentProject->operationalEnvironments.end(),
                [](const auto& oe) {
                    return !oe.heuristicData.mainHistogram.heuristicFilePath.empty();
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

        // Step 2: Convert to histograms
        else if (currentStep == 1) {
            ImGui::Text("Step 2: Convert raw data into histograms.");
            ImGui::TextWrapped("This will preprocess all uploaded files into histogram form for analysis.");

            // Check if all OEs already have converted files
            bool allConverted = std::all_of(
                m_currentProject->operationalEnvironments.begin(),
                m_currentProject->operationalEnvironments.end(),
                [](const auto& oe) {
                    return !oe.heuristicData.mainHistogram.convertedFilePath.empty();
                }
            );

            stepCompleted[1] = allConverted; // if true, "Next" can be clicked immediately

            if (ImGui::Button("Process Files", ImVec2(200, 0))) {
                if (m_onCommand) {
                    // Loop through all Operational Environments
                    for (size_t i = 0; i < m_currentProject->operationalEnvironments.size(); ++i) {
                        auto& oe = m_currentProject->operationalEnvironments[i];
                        auto& hist = oe.heuristicData.mainHistogram;

                        // Only process if convertedFilePath is not set but heuristicFilePath is available
                        if (hist.convertedFilePath.empty() && !hist.heuristicFilePath.empty()) {
                            m_onCommand(ProcessHistogramCommand{ static_cast<int>(i) });
                        }
                    }
                }

                stepCompleted[1] = true; // mark step as complete after issuing commands
            }
        }

        // Step 3: Run statistical tests
        else if (currentStep == 2) {
            ImGui::Text("Step 3: Run statistical tests on generated histograms.");
            ImGui::TextWrapped("Runs the non-IID test suite on all processed data sets.");

            ImVec2 runStatisticalTestSize(200, 0);

            if (ImGui::Button("Run Analysis", runStatisticalTestSize)) {
                if (m_onCommand) {
                    for (size_t i = 0; i < m_currentProject->operationalEnvironments.size(); ++i) {
                        auto& oe = m_currentProject->operationalEnvironments[i];
                        auto& hist = oe.heuristicData.mainHistogram;

                        // Only run tests if we have a converted file
                        if (!hist.convertedFilePath.empty()) {
                            hist.testsRunning = true;
                            hist.startTime = std::chrono::steady_clock::now();

                            m_onCommand(RunStatisticalTestCommand{
                                static_cast<int>(i),          // OE index
                                0,                            // Main Histogram index
                                hist.heuristicFilePath.string(),
                                std::shared_ptr<lib90b::NonIidResult>(
                                    &hist.entropyResults,
                                    [](lib90b::NonIidResult*){}), // shared_ptr wrapper
                                std::nullopt,                 // label (optional)
                                std::nullopt                  // additional params
                            });
                        }
                    }
                }

                stepCompleted[2] = true; // mark step as complete
            }
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
