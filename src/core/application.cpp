
#include <iostream>
#include <string>
#include <filesystem>

#include <imgui.h>

#include "config.h"
#include "application.h"
#include "../data/find_first_passing_decimation/find_first_passing_decimation.h"

namespace fs = std::filesystem;

bool Application::Initialize() {
    fs::path baseDir = fs::current_path();

    if (!dataManager.Initialize(&config, &currentProject)) {
        std::cerr << "Failed to initialize data manager" << std::endl;
        return false;
    }

    if (!uiManager.Initialize(&dataManager, &config, &currentProject)) {
        std::cerr << "Failed to initialize ui manager" << std::endl;
        return false;
    }

    uiManager.OnProjectChanged(currentProject);

    // Setup ImGui styling
    SetupImGuiStyle();
    LoadFonts();
    
    return true;
}

void Application::Update() {
    AppCommand cmd;
    while (commandQueue.Pop(cmd)) {
        std::visit([this](auto&& command) {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, NewProjectCommand>) {
                NewProject(command);
                uiManager.OnProjectChanged(currentProject);
            } else if constexpr (std::is_same_v<T, OpenProjectCommand>) {
                LoadProject(command.filePath);
                uiManager.OnProjectChanged(currentProject);
            } else if constexpr (std::is_same_v<T, SaveProjectCommand>) {
                SaveProject();
            } else if constexpr (std::is_same_v<T, AddOECommand>) {
                dataManager.AddOEToProject(currentProject, command.oeName, config);
                uiManager.OnProjectChanged(currentProject);
            } else if constexpr (std::is_same_v<T, DeleteOECommand>) {
                dataManager.DeleteOE(currentProject, command.oeIndex, config);
                uiManager.OnProjectChanged(currentProject);
            } else if constexpr (std::is_same_v<T, ProcessHistogramCommand>) {
                GetThreadPool().Enqueue([this, cmd = command] {
                    try {
                        auto& oe = currentProject.operationalEnvironments[cmd.oeIndex];

                        // 1. Convert decimal file first
                        lib90b::EntropyInputData entropyData;
                        if (!oe.heuristicData.mainHistogram.heuristicFilePath.empty()) {
                            if (!dataManager.ConvertDecimalFile(
                                    oe.heuristicData.mainHistogram.heuristicFilePath,
                                    oe.heuristicData.mainHistogram.convertedFilePath
                                    )) 
                            {
                                uiManager.PushNotification("Failed to convert file for statistical tests.", 5.0f, ImVec4(1,0,0,1));
                                return;
                            }
                        } else {
                            uiManager.PushNotification("No raw file uploaded to convert.", 5.0f, ImVec4(1,0,0,1));
                            return;
                        }

                        // 2. Process histogram
                        dataManager.processHistogramForProject(
                            currentProject,
                            cmd.oeIndex,
                            GetThreadPool(),
                            [this](const std::string& msg, float duration, ImVec4 color) {
                                uiManager.PushNotification(msg, duration, color);
                            }
                        );

                        uiManager.PushNotification("Histogram processing completed.", 3.0f, ImVec4(0,1,0,1));
                    } catch (const std::exception& e) {
                        uiManager.PushNotification(std::string("Histogram processing failed: ") + e.what(), 5.0f, ImVec4(1,0,0,1));
                    }
                });
            } else if constexpr (std::is_same_v<T, RunNonIidTestCommand>) {
                // Enqueue work
                GetThreadPool().Enqueue([this, cmd = command] {
                    try {
                        std::filesystem::path filepath = cmd.inputFile;
                        std::string linuxPath = toWslCommandPath(filepath);
                        std::string wslCmd = "wsl ea_non_iid -v " + linuxPath;

                        cmd.testTimer->StartTestsTimer();

                        std::string output = executeCommand(wslCmd);
                        
                        // Prepend input filename (without extension) to result filename
                        std::string resultFilename = filepath.stem().string() + "_nonIidResult.txt";
                        std::filesystem::path logFile = filepath.parent_path() / resultFilename;
                        writeStringToFile(output, logFile);

                        *cmd.result = output;
                        *cmd.outputFile = logFile;

                        if (!cmd.nonIidParsedResults->ParseResult(output)) {
                            // Failed to parse
                            uiManager.PushNotification("Warning: Could not parse test results", 5.0f, ImVec4(1,0.5,0,1));
                        }

                        cmd.testTimer->StopTestsTimer();

                        uiManager.PushNotification("Non-IID test completed.", 3.0f, ImVec4(0,1,0,1));
                    } catch (const std::exception& e) {
                        uiManager.PushNotification(std::string("Test failed: ") + e.what(), 5.0f, ImVec4(1,0,0,1));
                    }
                });
            } else if constexpr (std::is_same_v<T, RunRestartTestCommand>) {
                // Enqueue work
                GetThreadPool().Enqueue([this, cmd = command] {
                    try {
                        std::filesystem::path filepath = cmd.inputFile;
                        std::string linuxPath = toWslCommandPath(filepath);
                        std::string wslCmd = "wsl ea_restart -nv " + linuxPath + " " + std::to_string(cmd.minEntropy);

                        cmd.testTimer->StartTestsTimer();

                        std::string output = executeCommand(wslCmd);
                        
                        // Prepend input filename (without extension) to result filename
                        std::string resultFilename = filepath.stem().string() + "restartResult.txt";
                        std::filesystem::path logFile = filepath.parent_path() / resultFilename;
                        writeStringToFile(output, logFile);

                        *cmd.result = output;
                        *cmd.outputFile = logFile;

                        cmd.testTimer->StopTestsTimer();

                        uiManager.PushNotification("Restart test completed.", 3.0f, ImVec4(0,1,0,1));
                    } catch (const std::exception& e) {
                        uiManager.PushNotification(std::string("Test failed: ") + e.what(), 5.0f, ImVec4(1,0,0,1));
                    }
                });
            } else if constexpr (std::is_same_v<T, FindPassingDecimationCommand>) {
                GetThreadPool().Enqueue([this, cmd = command] {
                    try {
                        // Get reference to the OE
                        auto& oe = currentProject.operationalEnvironments[cmd.oeIndex];

                        // Start timer here in main thread
                        cmd.testTimer->StartTestsTimer();

                        // Run the decimation function
                        std::string result = findFirstPassingDecimation(cmd.inputFile);

                        // Write the result
                        if (cmd.output) {
                            *cmd.output = result;
                        }

                        oe.heuristicData.mainHistogram.firstPassingDecimationResult = result;
                        cmd.testTimer->StopTestsTimer();

                        uiManager.PushNotification("Find Passing Decimation completed.", 3.0f, ImVec4(0,1,0,1));
                    } catch (const std::exception& e) {
                        uiManager.PushNotification(std::string("Decimation failed: ") + e.what(), 5.0f, ImVec4(1,0,0,1));
                    }
                });
            }
        }, cmd);
    }
}

void Application::Render() {
    uiManager.Render();
}

void Application::Shutdown() {
    // Update last opened project in config
    if (!currentProject.name.empty()) {
        config.lastOpenedProject.name = currentProject.name;
        config.lastOpenedProject.path = currentProject.path;
        config.lastOpenedProject = currentProject;
        dataManager.SaveProject(currentProject, config);
    }
}

void Application::NewProject(const NewProjectCommand& formResult) {
    // Create project via DataManager
    fs::path projectFile = dataManager.NewProject(formResult.vendor, formResult.repo, formResult.projectName);

    if (projectFile.empty()) {
        std::cerr << "New project file name is empty!" << std::endl;
        return;
    }

    // Open project
    if (!LoadProject(projectFile.string())) {
        std::cerr << "Failed to open new project!" << std::endl;
    }
}

bool Application::LoadProject(const std::string& filePath) {
    if (filePath.empty()) return false;

    // Save current project before switching
    if (!currentProject.name.empty()) {
        dataManager.SaveProject(currentProject, config);
    }

    // Load the new project
    Project loadedProject = dataManager.LoadProject(filePath);
    if (loadedProject.name.empty()) {
        std::cerr << "Failed to load project: " << filePath << std::endl;
        return false;
    }

    currentProject = loadedProject;

    // Update last opened project in config
    config.lastOpenedProject = currentProject;

    // Persist app.json
    dataManager.saveAppConfig("../../data/app.json", config);

    return true;
}

void Application::SaveProject() {
    dataManager.SaveProject(currentProject, config);
}

void Application::ShowHelp() {
    //show_help_window = !show_help_window;
}

void Application::SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Styling
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 8);
    
    // Dark grey color scheme
    ImVec4* colors = style.Colors;
    
    // Main background colors
    colors[ImGuiCol_WindowBg] = Config::BACKGROUND_COLOR;                  // Dark grey window background
    colors[ImGuiCol_ChildBg] = Config::SLIGHTLY_LIGHTER_BACKGROUND_COLOR;  // Slightly lighter for child windows
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);         // Darker for popups
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);       // Menu bar background
    
    // Frame/input backgrounds
    colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);         // Input fields, etc.
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);  // Hovered state
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);   // Active state
    
    // Button colors
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.58f, 0.98f, 0.40f);          // Keep blue buttons
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.58f, 0.98f, 0.60f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.58f, 0.98f, 0.80f);
    
    // Header colors (for collapsing headers, etc.)
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    
    // Tab colors
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.58f, 0.98f, 0.40f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.58f, 0.98f, 0.60f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    
    // Text colors
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);            // Light grey text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);    // Disabled text
    
    // Border colors
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // Scrollbar colors
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    
    // Slider colors
    colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.58f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    
    // Selection colors
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.58f, 0.98f, 0.35f);
    
    // Separator colors
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    
    // Title bar colors
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    
    // Check mark colors
    colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.58f, 0.98f, 1.00f);
    
    // Progress bar colors
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.25f, 0.58f, 0.98f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
}

void Application::LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Load default font
    ImFontConfig config;
    config.MergeMode = false;
    config.PixelSnapH = true;
    io.Fonts->AddFontDefault();
    
    Config::fontH1 = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Regular.ttf", 28.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 28.0f, &iconConfig, icons_ranges);
    }

    Config::fontH1_Bold = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Bold.ttf", 28.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 28.0f, &iconConfig, icons_ranges);
    }

    Config::fontH2 = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Regular.ttf", 22.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 22.0f, &iconConfig, icons_ranges);
    }

    Config::fontH2_Bold = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Bold.ttf", 22.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 22.0f, &iconConfig, icons_ranges);
    }

    Config::fontH3 = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Regular.ttf", 18.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 18.0f, &iconConfig, icons_ranges);
    }

    Config::fontH3_Bold = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Bold.ttf", 18.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 18.0f, &iconConfig, icons_ranges);
    }

    Config::normal = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Roboto-Regular.ttf", 14.0f);
    {
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 };
        io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 14.0f, &iconConfig, icons_ranges);
    }

    static const ImWchar icons_ranges[] = { 0xf000, 0xf8ff, 0 }; // Font Awesome range
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    Config::icons = io.Fonts->AddFontFromFileTTF("../../assets/fonts/Font Awesome 6 Free-Solid-900.otf", 16.0f, &iconConfig, icons_ranges);

    io.Fonts->Build();
}