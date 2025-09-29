
#include "heuristic_manager.h"

#include <algorithm>

#include <lib90b/non_iid.h>

static bool ConvertDecimalFileToEntropyData(const std::string& filePath, 
                                     lib90b::EntropyInputData& outData,
                                     std::string& outBinaryFilePath)
{
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) return false;

    std::vector<uint8_t> symbols;
    std::string line;

    while (std::getline(inFile, line)) {
        try {
            double delta = std::stod(line);                  // parse decimal
            uint64_t sample = static_cast<uint64_t>(delta * 1e6); // scale if needed
            uint8_t symbol = static_cast<uint8_t>(sample & 0xFF); // mask bottom 8 bits
            symbols.push_back(symbol);
        } catch (...) {
            continue; // skip invalid lines
        }
    }

    if (symbols.empty()) return false;

    // Fill EntropyInputData
    outData.symbols = std::move(symbols);
    outData.word_size = 8;
    outData.alph_size = 256;

    // Prepare output file path based on input file
    fs::path inputPath(filePath);
    fs::path outPath = inputPath.parent_path() / (inputPath.stem().string() + "_converted.bin");

    // Save binary file
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(reinterpret_cast<const char*>(outData.symbols.data()), outData.symbols.size());
    outFile.close();

    outBinaryFilePath = outPath.string();
    return true;
}

static bool CreateSubHistogramFile(const std::string& inputFilePath,
                                   const HistogramRegion& region,
                                   std::string& outSubFilePath)
{
    std::ifstream inFile(inputFilePath);
    if (!inFile.is_open()) return false;

    fs::path inputPath(inputFilePath);
    std::string stem = inputPath.stem().string();
    fs::path outPath = inputPath.parent_path() / (stem + "_region" + std::to_string(region.regionIndex) + ".data");

    std::ofstream outFile(outPath);
    if (!outFile.is_open()) return false;

    std::string line;
    while (std::getline(inFile, line)) {
        try {
            double val = std::stod(line);
            if (val >= region.rect.X.Min && val <= region.rect.X.Max) {
                outFile << val << "\n";
            }
        } catch (...) {
            continue; // skip invalid lines
        }
    }

    inFile.close();
    outFile.close();

    outSubFilePath = outPath.string();
    return true;
}

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
    ImGui::BeginChild("SelectMainHistogramRegions", ImVec2(sidebarWidth, 460), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string selectMainHistogramRegionsTitle = std::string(u8"\uf0fe") + "  Selected Regions";
        ImGui::Text(selectMainHistogramRegionsTitle.c_str());
        ImGui::PopFont();

        ImGui::PushFont(Config::normal);

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
            std::string deleteButton = std::string(u8"\uf1f8");
            if (ImGui::Button(deleteButton.c_str())) {
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

        // Disable "Add Selection" if histogram has no data
        bool hasHistogram = std::any_of(
            oe->heuristicData.mainHistogram.binCounts.begin(),
            oe->heuristicData.mainHistogram.binCounts.end(),
            [](int c){ return c > 0; }
        );

        ImGui::BeginDisabled(!hasHistogram);
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

            region.regionIndex = static_cast<int>(oe->heuristicData.regions.size()) + 1;
            
            oe->heuristicData.regions.push_back(std::move(region));
        }
        ImGui::EndDisabled();

        ImGui::PopFont();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Main content
    ImGui::BeginChild("MainHistogram", ImVec2(mainWidth - 10.0f, 460), true);
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
                auto oe = GetSelectedOE();
                if (!oe || oe->heuristicData.heuristicFilePath.empty()) return;

                if (!ConvertDecimalFileToEntropyData(oe->heuristicData.heuristicFilePath,
                                                    oe->heuristicData.entropyData,
                                                    oe->heuristicData.convertedFilePath)) {
                    ImGui::OpenPopup("Error");
                    return;
                }

                oe->heuristicData.testsRunning = true;
                oe->heuristicData.startTime = std::chrono::steady_clock::now();

                if (m_onCommand) {
                    m_onCommand(RunStatisticalTestCommand{
                        oe->heuristicData.convertedFilePath,
                        std::shared_ptr<lib90b::NonIidResult>(&oe->heuristicData.entropyResults,
                                        [](lib90b::NonIidResult*){})
                    });
                }
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

        std::string plotLabel = "##MainHistogramPlot_" + oe->oeName;
        if (ImPlot::BeginPlot(plotLabel.c_str(), ImVec2(-1, 300))) {
            // Setup axes with auto-fit so they expand to the histogram range
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

                ImPlot::PlotBars("##MainHistogramSamples", xs.data(), ys.data(), hist.binCount, hist.binWidth);
            }

            // Draw regions as DragRects
            for (size_t i = 0; i < oe->heuristicData.regions.size(); ++i) {
                auto& region = oe->heuristicData.regions[i];
                ImPlotRect& rect = region.rect;

                ImPlotRect plotLimits = ImPlot::GetPlotLimits();

                // Keep the rectangle Y full height of the plot
                rect.Y.Min = plotLimits.Y.Min;
                rect.Y.Max = plotLimits.Y.Max;

                // Draw the draggable rectangle
                ImPlot::DragRect(static_cast<int>(i), 
                                &rect.X.Min, &rect.Y.Min,
                                &rect.X.Max, &rect.Y.Max,
                                region.color, 
                                ImPlotDragToolFlags_None);

                // Clamp X to histogram min/max after dragging
                rect.X.Min = std::max(rect.X.Min, static_cast<double>(oe->heuristicData.mainHistogram.minValue));
                rect.X.Max = std::min(rect.X.Max, static_cast<double>(oe->heuristicData.mainHistogram.maxValue));

                // Ensure min <= max
                if (rect.X.Min > rect.X.Max) std::swap(rect.X.Min, rect.X.Max);
            }

            ImPlot::EndPlot();
        }

        // Metadata Area
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

        ImGui::Separator();

        // Render non-iid test results if available
        ImGui::Text("Main Histogram Non-IID results:");
        if (oe->heuristicData.entropyResults.min_entropy.has_value()) {
            const auto& res = oe->heuristicData.entropyResults;

            if (res.H_original.has_value())
                ImGui::BulletText("H_original: %.6f bits", res.H_original.value());
            ImGui::SameLine();

            if (res.H_bitstring.has_value())
                ImGui::BulletText("H_bitstring: %.6f bits", res.H_bitstring.value());
            ImGui::SameLine();

            ImGui::BulletText("Min Entropy: %.6f bits", res.min_entropy.value());
        } else if (oe->heuristicData.testsRunning) {
            float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - oe->heuristicData.startTime).count();
            ImGui::Text("Running tests %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
        } else {
            ImGui::BulletText("Non-IID results not available");
        }

        ImGui::PopFont();
    }
    ImGui::EndChild();

    // Sub-Histograms
    ImGui::BeginChild("SubHistograms", ImVec2(fullWidth, 0), true);
    {
        ImGui::PushFont(Config::fontH3_Bold);
        std::string subHistogramsTitle = std::string(u8"\uf1fe") + "  Sub Histograms";
        ImGui::Text(subHistogramsTitle.c_str());
        ImGui::PopFont();
        ImGui::Separator();

        PrecomputedHistogram& hist = oe->heuristicData.mainHistogram;

        for (size_t i = 0; i < oe->heuristicData.regions.size(); ++i) {
            auto& region = oe->heuristicData.regions[i];

            // Convert rect range to bin indexes
            int binMin = static_cast<int>((region.rect.X.Min - hist.minValue) / hist.binWidth);
            int binMax = static_cast<int>((region.rect.X.Max - hist.minValue) / hist.binWidth);
            binMin = std::clamp(binMin, 0, hist.binCount - 1);
            binMax = std::clamp(binMax, 0, hist.binCount - 1);
            if (binMin > binMax) std::swap(binMin, binMax);

            // Prepare sub-histogram data
            std::vector<double> xs, ys;
            xs.reserve(binMax - binMin + 1);
            ys.reserve(binMax - binMin + 1);
            for (int b = binMin; b <= binMax; b++) {
                double xCenter = hist.minValue + (b + 0.5) * hist.binWidth;
                xs.push_back(xCenter);
                ys.push_back(static_cast<double>(hist.binCounts[b]));
            }

            // Each sub-histogram has its own child spanning full width
            std::string childLabel = "SubHistogramChild_" + std::to_string(i);
            ImGui::BeginChild(childLabel.c_str(), ImVec2(-1, 250), true);
            {
                // Left column (metadata + buttons)
                ImGui::BeginChild(("SubMeta_" + std::to_string(i)).c_str(), ImVec2(sidebarWidth - 10.0f, -1), false);
                {
                    std::string regionTitle = "Region " + std::to_string(region.regionIndex);

                    // --- Title on the left ---
                    ImGui::PushFont(Config::fontH3_Bold);
                    ImGui::Text(regionTitle.c_str());
                    ImGui::PopFont();

                    // --- Buttons on the right ---
                    ImVec2 runTestSize(30, 30);
                    ImVec2 configSize(30, 30);
                    float spacing = 6.0f;
                    float totalWidth = runTestSize.x + spacing + configSize.x;

                    float regionWidth = ImGui::GetContentRegionAvail().x;
                    float startX = regionWidth - totalWidth + ImGui::GetStyle().ItemSpacing.x;

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(startX - 16.0f);

                    ImGui::PushFont(Config::normal);

                    // Run Test button
                    ImGui::PushStyleColor(ImGuiCol_Button,        Config::MINT_BUTTON.normal);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::MINT_BUTTON.hovered);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::MINT_BUTTON.active);
                    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
                    {
                        
                        if (ImGui::Button(std::string(u8"\uf83e").c_str())) {
                            auto oe = GetSelectedOE();
                            if (!oe) return;

                            if (!CreateSubHistogramFile(oe->heuristicData.heuristicFilePath, region, region.subFilePath)) {
                                ImGui::OpenPopup("Error");
                                return;
                            }

                            // Now run the usual conversion & statistical test
                            if (!ConvertDecimalFileToEntropyData(region.subFilePath,
                                                                region.entropyData,
                                                                region.convertedFilePath)) {
                                ImGui::OpenPopup("Error");
                                return;
                            }

                            region.testsRunning = true;
                            region.startTime = std::chrono::steady_clock::now();

                            if (m_onCommand) {
                                m_onCommand(RunStatisticalTestCommand{
                                    region.convertedFilePath,
                                    std::shared_ptr<lib90b::NonIidResult>(&region.entropyResults, [](lib90b::NonIidResult*){}) 
                                });
                            }
                        }
                    }
                    ImGui::PopStyleColor(4);

                    ImGui::SameLine(0, spacing);

                    // Config (cog) button
                    ImGui::PushStyleColor(ImGuiCol_Button,        Config::WHITE_BUTTON.normal);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::WHITE_BUTTON.hovered);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::WHITE_BUTTON.active);
                    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
                    {
                        if (ImGui::Button((std::string(u8"\uf013").c_str()))) {
                            // TODO: config
                        }
                    }
                    ImGui::PopStyleColor(4);

                    // --- Metadata below ---
                    ImGui::Separator();
                    ImGui::Text("%s Metadata:", regionTitle.c_str());
                    ImGui::BulletText("Min: %d", (int)region.rect.X.Min);
                    ImGui::BulletText("Max: %d", (int)region.rect.X.Max);

                    int binStart = static_cast<int>((region.rect.X.Min - hist.minValue) / hist.binWidth);
                    int binEnd   = static_cast<int>((region.rect.X.Max - hist.minValue) / hist.binWidth);
                    binStart = std::max(0, binStart);
                    binEnd   = std::min(hist.binCount - 1, binEnd);
                    int subBinCount = std::max(0, binEnd - binStart + 1);

                    ImGui::BulletText("Bins: %d", subBinCount);
                    ImGui::BulletText("Bin Width: %.2f", hist.binWidth);

                    ImGui::Separator();
                    ImGui::Text("%s Non-IID results:", regionTitle.c_str());
                    if (region.entropyResults.min_entropy.has_value()) {
                        const auto& res = region.entropyResults;
                        if (res.H_original.has_value())
                            ImGui::BulletText("H_original: %.6f bits", res.H_original.value());
                        if (res.H_bitstring.has_value())
                            ImGui::BulletText("H_bitstring: %.6f bits", res.H_bitstring.value());
                        ImGui::BulletText("Min Entropy: %.6f bits", res.min_entropy.value());
                    } else if (region.testsRunning) {
                        float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - region.startTime).count();
                        ImGui::Text("Running %.1fs %c", t, "|/-\\"[static_cast<int>(t*4) % 4]);
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
                        ImPlot::SetupAxes("Value", "Frequency",
                                        ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

                        if (!xs.empty()) {
                            // Copy region color but make fully opaque
                            ImVec4 opaqueColor = region.color;
                            opaqueColor.w = 1.0f; // Force to not be transparent

                            ImPlot::SetNextFillStyle(opaqueColor);
                            ImPlot::PlotBars("##SubHistogramSamples", xs.data(), ys.data(), (int)xs.size(), hist.binWidth);
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
