
#include <imgui.h>

#include "ui_manager.h"
#include "../core/config.h"

bool UIManager::Initialize(DataManager* dataManager) {
    m_dataManager = dataManager;
    
    return true;
}

void UIManager::Render(Tabs& activeTab, Project& project) {
    float fullWidth = ImGui::GetContentRegionAvail().x;
    float fullHeight = ImGui::GetContentRegionAvail().y;

    float sidebarWidth = fullWidth * 0.20f;
    float mainWidth    = fullWidth * 0.80f;

    // Sidebar
    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);
    {
        RenderSidebar(activeTab, project);
    }
    ImGui::EndChild();

    // Move to right of sidebar
    ImGui::SameLine();

    // Main content region
    ImGui::BeginChild("MainArea", ImVec2(mainWidth, 0), true);
    {
        RenderMainContent(activeTab, project);
    }
    ImGui::EndChild();
}

void UIManager::RenderSidebar(Tabs& activeTab, Project& project) {

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
        uiState.saveProjectRequested = true;
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
        std::string projectLabel = std::string(u8"\uf1c0") + " " + project.name;
        ImGui::Text(projectLabel.c_str());
        ImGui::PopFont();
    }
    ImGui::EndChild();

    // Buttons to swap between statisicatl assessment and heuristic assessment
    if (ImGui::Selectable("Statistical Assessment", activeTab == Tabs::StatisticalAssessment))
        activeTab = Tabs::StatisticalAssessment;
    if (ImGui::Selectable("Heuristic Assessment", activeTab == Tabs::HeuristicAssessment))
        activeTab = Tabs::HeuristicAssessment;

    // TODO: If project is loaded & the project has OEs, then list them as a card else do nothing
}

void UIManager::RenderMainContent(Tabs& activeTab, Project& project) {
    switch (activeTab) {
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
                if (onNewProject) onNewProject();
            }
            if (ImGui::MenuItem("Open Project")) {
                if (onOpenProject) onOpenProject();
            }
            if (ImGui::MenuItem("Save Project")) {
                if (onSaveProject) onSaveProject();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Toggle Fullscreen", "F11")) {
                if (onToggleFullscreen) onToggleFullscreen();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Show Help", "F1")) {
                if (onShowHelp) onShowHelp();
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

void UIManager::RenderMainWindow(Tabs& activeTab, Project& project) {
    // Main application window
    ImGui::Begin(Config::APPLICATION_TITLE, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    Render(activeTab, project);
    
    ImGui::End();

}

void UIManager::RenderHelpWindow(bool show_help_window) {
    // Render help window if needed
    if (show_help_window) {
        ImGui::Begin("Help", &show_help_window);
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

NewProjectFormResult UIManager::RenderNewProjectPopup(const std::vector<std::string>& vendors) {
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
        if (ImGui::BeginCombo("##vendor", vendors[selectedVendorIndex].c_str())) {
            for (int n = 0; n < vendors.size(); n++) {
                bool isSelected = (selectedVendorIndex == n);
                if (ImGui::Selectable(vendors[n].c_str(), isSelected))
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
            result.vendor = vendors[selectedVendorIndex];
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

LoadProjectFormResult UIManager::RenderLoadProjectPopup(const std::vector<Project>& savedProjects) {
    LoadProjectFormResult result;

    // Keep calling OpenPopup every frame until modal is shown
    if (uiState.loadProjectPopupOpen) {
        ImGui::OpenPopup("Load Project");
    }

    if (ImGui::BeginPopupModal("Load Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a Project:");

        // List saved projects in a selectable list
        static int selectedIndex = 0;
        for (int i = 0; i < savedProjects.size(); i++) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(savedProjects[i].name.c_str(), isSelected)) {
                selectedIndex = i;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }

        if (ImGui::Button("Load")) {
            if (!savedProjects.empty()) {
                result.submitted = true;
                result.filePath = savedProjects[selectedIndex].path;
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
