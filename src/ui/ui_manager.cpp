
#include <imgui.h>

#include "ui_manager.h"
#include "../core/config.h"

bool UIManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project) {
    m_dataManager = dataManager;
    m_config = config;
    m_currentProject = project;
    
    return true;
}

void UIManager::Render() {
    RenderMenuBar();
    RenderMainWindow();
    RenderHelpWindow();
    RenderPopups();
    RenderNotifications();
}

// Main Content
void UIManager::RenderMainWindow() {
    bool open = true;
    if (ImGui::Begin(Config::APPLICATION_TITLE, &open,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus)) 
    {
        float fullWidth = ImGui::GetContentRegionAvail().x;
        float fullHeight = ImGui::GetContentRegionAvail().y;

        float sidebarWidth = fullWidth * 0.20f;
        float mainWidth    = fullWidth * 0.80f;

        // Sidebar
        ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);
        {
            RenderSidebar();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Main content
        ImGui::BeginChild("MainArea", ImVec2(mainWidth, 0), true);
        {
            RenderMainContent();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void UIManager::RenderSidebar() {

    ImGui::PushFont(Config::fontH1);
    ImGui::Text(Config::APPLICATION_TITLE);
    ImGui::PopFont();

    ImGui::PushFont(Config::fontH3);
    ImGui::Text(Config::APPLICATION_VERSION);
    ImGui::PopFont();

    ImGui::PushFont(Config::normal);
    ImGui::TextWrapped("Statistical and heuristic analysis tool for entropy sources");
    ImGui::PopFont();

    ImGui::Spacing();

    ImGui::PushFont(Config::fontH3);
    std::string newProjectButton = std::string(u8"\uf067") + "  New Project"; // \uf067 is the plus icon
    if (ImGui::Button(newProjectButton.c_str())) {
        uiState.newProjectPopupOpen = true;
    }
    ImGui::PopFont();

    ImGui::PushFont(Config::fontH3);
    std::string loadProjectButton = std::string(u8"\uf07c") + "  Load Project";
    if(ImGui::Button(loadProjectButton.c_str())) {
        uiState.loadProjectPopupOpen = true;
    }
    ImGui::PopFont();

    ImGui::PushFont(Config::fontH3);
    std::string saveProjectButton = std::string(u8"\uf0c7") + "  Save Project"; // \uf0c7 is the floppy disk icon
    if (ImGui::Button(saveProjectButton.c_str())) {
        commandQueue.Push(SaveProjectCommand{});
        PushNotification("Project saved!", 3.0f, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    ImGui::PopFont();

    // Card with current project name
    float sidebarWidth = ImGui::GetContentRegionAvail().x;
    float projectBoxHeight = 60.0f; // whatever height you want

    ImGui::BeginChild("ProjectInfo", ImVec2(sidebarWidth, projectBoxHeight), true);
    {
        float textHeight = ImGui::GetTextLineHeight();
        float yOffset = (projectBoxHeight - textHeight) * 0.5f;
        if (yOffset > 0) ImGui::SetCursorPosY(yOffset);

        ImGui::PushFont(Config::fontH3);
        std::string projectLabel = std::string(u8"\uf1c0") + " " + m_currentProject->name;
        ImGui::Text(projectLabel.c_str());
        ImGui::PopFont();
    }
    ImGui::EndChild();

    // Buttons to swap between statisicatl assessment and heuristic assessment
    if (ImGui::Selectable("Statistical Assessment", uiState.activeTab == Tabs::StatisticalAssessment))
        uiState.activeTab = Tabs::StatisticalAssessment;
    if (ImGui::Selectable("Heuristic Assessment", uiState.activeTab == Tabs::HeuristicAssessment))
        uiState.activeTab = Tabs::HeuristicAssessment;

    // TODO: If project is loaded & the project has OEs, then list them as a card else do nothing
}

// Render Utility
void UIManager::RenderMainContent() {
    switch (uiState.activeTab) {
        case Tabs::StatisticalAssessment:
            ImGui::Text("This is Page A");
            break;
        case Tabs::HeuristicAssessment:
            ImGui::Text("This is Page B");
            break;
    }
}

void UIManager::RenderMenuBar() {
    // Render main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                uiState.newProjectPopupOpen = true;
            }
            if (ImGui::MenuItem("Load Project")) {
                uiState.loadProjectPopupOpen = true;
            }
            if (ImGui::MenuItem("Save Project")) {
                commandQueue.Push(SaveProjectCommand{});
                PushNotification("Project saved!", 3.0f, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            // TODO: Add options
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Show Help", "F1")) {
                uiState.showHelpWindow = true;
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Calculate main window position and size
    float menu_bar_height = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - menu_bar_height));
}

void UIManager::RenderHelpWindow() {
    // Render help window if needed
    if (uiState.showHelpWindow) {
        ImGui::Begin("Help", &uiState.showHelpWindow);
        std::string title = std::string(Config::APPLICATION_TITLE) + " Help";
        ImGui::Text(title.c_str());
        ImGui::Separator();
        ImGui::Text("Keyboard Shortcuts:");
        ImGui::Text("Ctrl+N: New Project");
        ImGui::Text("Ctrl+O: Open Project");
        ImGui::Text("Ctrl+S: Save Project");
        ImGui::Text("F11: Toggle Fullscreen");
        ImGui::Text("F1: Show/Hide Help");
        ImGui::End();
    }
}

// Notifications
void UIManager::PushNotification(const std::string& msg, float duration, ImVec4 color) {
    notifications.push_back({ msg, duration, color });
}

void UIManager::RenderNotifications() {
    for (auto it = notifications.begin(); it != notifications.end(); ) {
        ImGui::SetNextWindowPos(ImVec2(10, 10 + 50 * std::distance(notifications.begin(), it)), ImGuiCond_Always);
        ImGui::Begin(("Notification##" + std::to_string(std::distance(notifications.begin(), it))).c_str(),
                        nullptr,
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextColored(it->color, "%s", it->message.c_str());
        ImGui::End();

        it->duration -= ImGui::GetIO().DeltaTime;
        if (it->duration <= 0.0f) {
            it = notifications.erase(it);
        } else {
            ++it;
        }
    }
}

// Popups
void UIManager::RenderPopups(){ 
    if (uiState.loadProjectPopupOpen) {
        LoadProjectFormResult result = RenderLoadProjectPopup();

        if (result.submitted) {
            OpenProjectCommand cmd { result.filePath };
            commandQueue.Push(std::move(cmd));
        }
    }

    if (uiState.newProjectPopupOpen) {
        NewProjectFormResult result = RenderNewProjectPopup();

        if (result.submitted) {
            NewProjectCommand cmd {
                result.vendor,
                result.repo,
                result.projectName
            };
            commandQueue.Push(std::move(cmd));
        }
    }
}

NewProjectFormResult UIManager::RenderNewProjectPopup() {
    static int selectedVendorIndex = 0;
    static char repoName[128] = "";
    static char projectName[128] = "";

    NewProjectFormResult result;

    // Keep calling OpenPopup every frame until modal is shown
    if (uiState.newProjectPopupOpen) {
        ImGui::OpenPopup("New Project");
    }

    if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Vendor:");
        if (ImGui::BeginCombo("##vendor", m_config->vendorsList[selectedVendorIndex].c_str())) {
            for (int n = 0; n < m_config->vendorsList.size(); n++) {
                bool isSelected = (selectedVendorIndex == n);
                if (ImGui::Selectable(m_config->vendorsList[n].c_str(), isSelected))
                    selectedVendorIndex = n;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Repository Name:");
        ImGui::InputText("##repo", repoName, IM_ARRAYSIZE(repoName));

        ImGui::Text("Project Name:");
        ImGui::InputText("##project", projectName, IM_ARRAYSIZE(projectName));

        if (ImGui::Button("Create")) {
            result.submitted = true;
            result.vendor = m_config->vendorsList[selectedVendorIndex];
            result.repo = repoName;
            result.projectName = projectName;
            ImGui::CloseCurrentPopup();
            uiState.newProjectPopupOpen = false; // reset flag
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            result.submitted = false;
            ImGui::CloseCurrentPopup();
            uiState.newProjectPopupOpen = false; // reset flag
        }

        ImGui::EndPopup();
    }

    return result;
}

LoadProjectFormResult UIManager::RenderLoadProjectPopup() {
    LoadProjectFormResult result;

    // Keep calling OpenPopup every frame until modal is shown
    if (uiState.loadProjectPopupOpen) {
        ImGui::OpenPopup("Load Project");
    }

    if (ImGui::BeginPopupModal("Load Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a Project:");

        // List saved projects in a selectable list
        static int selectedIndex = 0;
        for (int i = 0; i < m_config->savedProjects.size(); i++) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(m_config->savedProjects[i].name.c_str(), isSelected)) {
                selectedIndex = i;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }

        if (ImGui::Button("Load")) {
            if (!m_config->savedProjects.empty()) {
                result.submitted = true;
                result.filePath = m_config->savedProjects[selectedIndex].path;
            }
            ImGui::CloseCurrentPopup();
            uiState.loadProjectPopupOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            result.submitted = false;
            ImGui::CloseCurrentPopup();
            uiState.loadProjectPopupOpen = false;
        }

        ImGui::EndPopup();
    }

    return result;
}
