
#include <imgui.h>

#include "ui_manager.h"
#include "../core/config.h"

bool UIManager::Initialize(DataManager* dataManager, Config::AppConfig* config, Project* project) {
    m_dataManager = dataManager;
    m_config = config;
    m_currentProject = project;
    
    heuristicManager.Initialize(dataManager, config, project);

    // Set callbacks so heuristicManager can notify or request actions
    heuristicManager.SetCommandCallback([this](AppCommand cmd) {
        commandQueue.Push(std::move(cmd));
    });

    heuristicManager.SetNotificationCallback([this](const std::string& msg, float duration, ImVec4 color) {
        PushNotification(msg, duration, color);
    });

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
    ImGui::PushFont(Config::fontH1_Bold);
    ImGui::TextWrapped(Config::APPLICATION_TITLE);
    ImGui::PopFont();

    ImGui::PushFont(Config::fontH3);
    ImGui::Text(Config::APPLICATION_VERSION);
    ImGui::PopFont();

    ImGui::PushFont(Config::normal);
    ImGui::TextWrapped("Statistical and heuristic analysis tool for entropy sources");
    ImGui::PopFont();

    ImGui::Spacing();

    ImGui::PushFont(Config::fontH3);

    float sidebarWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 buttonSize(sidebarWidth * 0.7f, 0);

    ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
    {
        std::string newProjectButton = std::string(u8"\uf067") + "  New Project";
        if (ImGui::Button(newProjectButton.c_str(), buttonSize)) {
            uiState.newProjectPopupOpen = true;
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::PushStyleColor(ImGuiCol_Button,        Config::ORANGE_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::ORANGE_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::ORANGE_BUTTON.active);
    {
        std::string loadProjectButton = std::string(u8"\uf07c") + "  Load Project";
        if (ImGui::Button(loadProjectButton.c_str(), buttonSize)) {
            uiState.loadProjectPopupOpen = true;
        }
    }
    ImGui::PopStyleColor(3);
    
    ImGui::PushStyleColor(ImGuiCol_Button,        Config::RED_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::RED_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::RED_BUTTON.active);
    {
        std::string saveProjectButton = std::string(u8"\uf0c7") + "  Save Project";
        if (ImGui::Button(saveProjectButton.c_str(), buttonSize)) {
            commandQueue.Push(AppCommand(SaveProjectCommand{}));
            PushNotification("Project saved!", 3.0f, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::PopFont();

    ImGuiSpacing(2);

    /* Card with current project name */
    float projectBoxHeight = 50.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Config::LIGHT_BACKGROUND_COLOR);
    ImGui::BeginChild("ProjectInfo", ImVec2(sidebarWidth, projectBoxHeight), true, ImGuiWindowFlags_NoScrollbar);
    {
        float padding = 10.0f;
        float iconColumnWidth = 40.0f; 
        float iconLeftPadding = 15.0f;
        float textColumnWidth = sidebarWidth - iconColumnWidth - 2 * padding;

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, iconColumnWidth);

        /* Right column: Icon */
        float cardHeight = ImGui::GetContentRegionAvail().y;
        float yOffset = (cardHeight) * 0.5f;
        if (yOffset > 0) ImGui::SetCursorPosY(yOffset);

        ImGui::SetCursorPosX(iconLeftPadding);
        ImGui::PushFont(Config::fontH3);
        ImGui::Text(u8"\uf1c0"); // Database icon
        ImGui::PopFont();

        ImGui::NextColumn();
        ImGui::SetColumnWidth(1, textColumnWidth);

        /* Right column: Text */
        // Reduce vertical spacing between label and project name
        float spacingBackup = ImGui::GetStyle().ItemSpacing.y; // save default
        ImGui::GetStyle().ItemSpacing.y = 1.0f; // set to a smaller spacing


        ImGui::PushFont(Config::fontH3_Bold); 
        ImGui::Text("Current Project"); 
        ImGui::PopFont();

        ImGui::PushFont(Config::normal);
        ImGui::TextWrapped("%s", m_currentProject->name.c_str());
        ImGui::PopFont();

        ImGui::GetStyle().ItemSpacing.y = spacingBackup; // restore default

        ImGui::Columns(1);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGuiSpacing(2);

    /* Statistics/Heuristic Button selector */
    float padding = ImGui::GetStyle().ItemSpacing.x;
    float projectButtonWidth = (sidebarWidth - padding) * 0.5f;
    ImVec2 projectButtonSize(projectButtonWidth, 0);

    ImGui::PushFont(Config::fontH3);

    bool isStatActive = (uiState.activeTab == Tabs::StatisticalAssessment);
    ImVec4 statColor = isStatActive ? Config::PURPLE_BUTTON.hovered : Config::PURPLE_BUTTON.normal;

    // First button: Statistical Assessment
    ImGui::PushStyleColor(ImGuiCol_Button,        statColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::PURPLE_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::PURPLE_BUTTON.active);
    {
        std::string statisticalButton = std::string(u8"\uf1ec") + "  Statistical";
        if (ImGui::Button(statisticalButton.c_str(), projectButtonSize)) {
            uiState.activeTab = Tabs::StatisticalAssessment;
        }
    }    
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, padding);

    // Second button: Heuristic Assessment
    bool isHeuristicActive = (uiState.activeTab == Tabs::HeuristicAssessment);
    ImVec4 heurColor = isHeuristicActive ? Config::PURPLE_BUTTON.hovered : Config::PURPLE_BUTTON.normal;

    ImGui::PushStyleColor(ImGuiCol_Button,        heurColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::PURPLE_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::PURPLE_BUTTON.active);
    {
        std::string heursiticButton = std::string(u8"\uf1fe") + "  Heuristic";
        if (ImGui::Button(heursiticButton.c_str(), projectButtonSize)) {
            uiState.activeTab = Tabs::HeuristicAssessment;
        }
    }    
    ImGui::PopStyleColor(3);

    ImGui::PopFont();

    ImGuiSpacing(2);

    /* Operational Environments */
    ImGui::PushFont(Config::fontH3_Bold);
    ImGui::Text("Operational Environments");
    ImGui::PopFont();

    ImGui::Spacing();
    
    ImGui::PushFont(Config::fontH3);

    float oeButtonHeight = 50.0f;
    float oeButtonWidth = sidebarWidth * 1.0f;

    for (size_t i = 0; i < m_currentProject->operationalEnvironments.size(); ++i) {
        const auto& oe = m_currentProject->operationalEnvironments[i];
        bool isActive = (uiState.selectedOEIndex == (int)i);

        ImVec4 bgColor = isActive ? Config::GREY_BUTTON.hovered : Config::GREY_BUTTON.normal;
        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREY_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Config::GREY_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
        {
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 prevAlign = style.ButtonTextAlign;

            style.ButtonTextAlign = ImVec2(0.0f, 0.5f); // Set text to left aligned
            std::string oeNameButton = std::string(u8"\uf2db") + " " + oe.oeName;
            if (ImGui::Button(oeNameButton.c_str(), ImVec2(oeButtonWidth, oeButtonHeight))) {
                uiState.selectedOEIndex = (int)i;
            }
            
            style.ButtonTextAlign = prevAlign; // restore to center aligned
        }
        ImGui::PopStyleColor(4);

        ImGui::Spacing();
    }
    ImGui::PopFont();

    /* Add OE Button + OE Settings cog */
    ImGui::PushFont(Config::fontH3);

    float oeSettingButtonPadding = ImGui::GetStyle().ItemSpacing.x;
    float addOEWidth = sidebarWidth * 0.7f;
    float cogWidth = sidebarWidth - addOEWidth - oeSettingButtonPadding;
    ImVec2 addOEButtonSize(addOEWidth, 0);
    ImVec2 cogButtonSize(cogWidth, 0);

    // Add OE Button
    ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
    {
        std::string addOEButton = std::string(u8"\uf067"); // '+' icon
        if (ImGui::Button(addOEButton.c_str(), addOEButtonSize)) {
            uiState.addOEPopupOpen = true;
        }
    }
    ImGui::PopStyleColor(3);

    // Cog button
    ImGui::SameLine(0, oeSettingButtonPadding);
    ImGui::PushStyleColor(ImGuiCol_Button,        Config::WHITE_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::WHITE_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::WHITE_BUTTON.active);
    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
    {
        ImGui::BeginDisabled(uiState.selectedOEIndex < 0);
        std::string cogButton = std::string(u8"\uf013"); // cog icon
        if (ImGui::Button(cogButton.c_str(), cogButtonSize)) {
            if (uiState.selectedOEIndex >= 0) {
                uiState.editOEPopupOpen = true;
            }
        }
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor(4);

    ImGui::PopFont();

}

// Render Utility
void UIManager::RenderMainContent() {
    switch (uiState.activeTab) {
        case Tabs::StatisticalAssessment:
            ImGui::Text("Statistical Assessment Page");
            break;
        case Tabs::HeuristicAssessment:
            heuristicManager.Render();
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
                commandQueue.Push(AppCommand(SaveProjectCommand{}));
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
        ImGui::SetNextWindowPos(
            ImVec2(10.0f, 10.0f + 50.0f * static_cast<float>(std::distance(notifications.begin(), it))),
            ImGuiCond_Always
        );
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

    if (uiState.addOEPopupOpen) {
        AddOEFormResult result = RenderAddOEPopup();

        if (result.submitted) {
            AddOECommand cmd { result.oeName };
            commandQueue.Push(std::move(cmd));
        }
    }

    if (uiState.editOEPopupOpen) {
        EditOEFormResult result = RenderEditOEPopup();

        if (result.submitted) {
            // Update OE name in the project
            auto& oe = m_currentProject->operationalEnvironments[uiState.selectedOEIndex];
            oe.oeName = result.newName;

            // Update project.json / save app config here
            SaveProjectCommand cmd {};
            commandQueue.Push(std::move(cmd));

            uiState.editOEPopupOpen = false;
        }

        if (result.deleted) {
            DeleteOECommand del{ uiState.selectedOEIndex };
            commandQueue.Push(del);

            uiState.editOEPopupOpen = false;
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

AddOEFormResult UIManager::RenderAddOEPopup() {
    AddOEFormResult result;

    // Keep calling OpenPopup every frame until modal is shown
    if (uiState.addOEPopupOpen) {
        ImGui::OpenPopup("Add Operational Environment");
    }

    if (ImGui::BeginPopupModal("Add Operational Environment", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter a name for the new OE:");

        static char oeNameBuffer[128] = "";
        ImGui::InputText("OE Name", oeNameBuffer, IM_ARRAYSIZE(oeNameBuffer));

        // Buttons
        if (ImGui::Button("Add")) {
            result.submitted = true;
            result.oeName = oeNameBuffer;

            ImGui::CloseCurrentPopup();
            uiState.addOEPopupOpen = false;
            std::fill(std::begin(oeNameBuffer), std::end(oeNameBuffer), 0); // clear input
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            result.submitted = false;

            ImGui::CloseCurrentPopup();
            uiState.addOEPopupOpen = false;
            std::fill(std::begin(oeNameBuffer), std::end(oeNameBuffer), 0); // clear input
        }

        ImGui::EndPopup();
    }

    return result;
}

EditOEFormResult UIManager::RenderEditOEPopup() {
    EditOEFormResult result;

    if (!uiState.editOEPopupOpen) return result;
    ImGui::OpenPopup("Edit Operational Environment");

    if (uiState.selectedOEIndex < 0 || uiState.selectedOEIndex >= (int)m_currentProject->operationalEnvironments.size())
        return result;

    auto& oe = m_currentProject->operationalEnvironments[uiState.selectedOEIndex];

    static char oeNameBuffer[256];
    static bool initialized = false;

    if (!initialized) {
        oe.oeName.copy(oeNameBuffer, sizeof(oeNameBuffer) - 1);
        oeNameBuffer[sizeof(oeNameBuffer) - 1] = '\0';
        initialized = true;
    }

    ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Edit Operational Environment", nullptr, ImGuiWindowFlags_NoResize)) {

        // Title
        ImGui::PushFont(Config::fontH2_Bold);
        ImGui::Text("Edit OE");
        ImGui::PopFont();
        ImGui::Spacing();

        // Input field
        ImGui::PushFont(Config::normal);
    
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Name:");
        ImGui::SameLine();

        ImGui::InputText("##OEName", oeNameBuffer, sizeof(oeNameBuffer));

        ImGui::PopFont();

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // Action buttons
        float buttonWidth = 120.0f;
        float spacing = 10.0f;

        // Save button
        ImGui::PushFont(Config::fontH3);
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREEN_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREEN_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREEN_BUTTON.active);
        if (ImGui::Button((std::string(u8"\uf0c7 ") + "Save").c_str(), ImVec2(buttonWidth, 0))) {
            result.submitted = true;
            result.newName = std::string(oeNameBuffer);
            ImGui::CloseCurrentPopup();
            uiState.editOEPopupOpen = false;
            initialized = false;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, spacing);

        // Cancel button
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREY_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREY_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.active);
        ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
        if (ImGui::Button((std::string(u8"\uf00d ") + "Cancel").c_str(), ImVec2(buttonWidth, 0))) {
            result.submitted = false;
            ImGui::CloseCurrentPopup();
            uiState.editOEPopupOpen = false;
            initialized = false;
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0, spacing);

        // Delete button
        static bool confirmDeleteOEPopupOpen = false;
        ImGui::PushStyleColor(ImGuiCol_Button,        Config::RED_BUTTON.normal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::RED_BUTTON.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::RED_BUTTON.active);
        if (ImGui::Button((std::string(u8"\uf1f8 ") + "Delete").c_str(), ImVec2(buttonWidth, 0))) {
            confirmDeleteOEPopupOpen = true;
            ImGui::OpenPopup("Confirm Delete OE");
        }
        ImGui::PopStyleColor(3);

        // Confirm Delete Modal
        if (confirmDeleteOEPopupOpen) {
            ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
            if (ImGui::BeginPopupModal("Confirm Delete OE", nullptr, ImGuiWindowFlags_NoResize)) {
                ImGui::PushFont(Config::fontH3_Bold);
                ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), (std::string(u8"\uf1f8 ") + "Delete OE?").c_str());
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::TextWrapped("Are you sure you want to delete OE \"%s\"?\nThis action cannot be undone.", oe.oeName.c_str());
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                if (ImGui::Button("Yes, delete", ImVec2(buttonWidth, 0))) {
                    result.deleted = true;
                    confirmDeleteOEPopupOpen = false;
                    uiState.editOEPopupOpen = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No", ImVec2(buttonWidth, 0))) {
                    confirmDeleteOEPopupOpen = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::PopFont();

        ImGui::EndPopup();
    }

    return result;
}

//UI Elements
void UIManager::ImGuiSpacing(int count) {
    ImGui::Dummy(ImVec2(0.0f, count * ImGui::GetStyle().ItemSpacing.y));
}

// Utility
void UIManager::OnProjectChanged(Project project) {
    if (!project.operationalEnvironments.empty()) {
        uiState.selectedOEIndex = 0;
    } else {
        uiState.selectedOEIndex = -1;
    }
}