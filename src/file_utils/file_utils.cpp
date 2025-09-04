
#include "file_utils.h"

void from_json(const json& j, Project& p) {
    j.at("projectName").get_to(p.name);
    j.at("projectPath").get_to(p.path);
}

void to_json(json& j, const Project& p) {
    j = json{
        {"projectName", p.name},
        {"projectPath", p.path}
    };
}

namespace Config {
    void from_json(const json& j, Config::AppConfig& c) {
        j.at("lastOpenedProject").get_to(c.lastOpenedProject);
        j.at("savedProjects").get_to(c.savedProjects);
    }

    void to_json(json& j, const Config::AppConfig& c) {
        j = json{
            {"lastOpenedProject", c.lastOpenedProject},
            {"savedProjects", c.savedProjects}
        };
    }
}

std::optional<std::string> FileSelector(
    const std::string& dialogKey,     // Unique key for this dialog
    const std::string& buttonLabel,   // Label for the button that opens dialog
    const std::string& fileFilters,   // e.g. ".txt,.bin"
    const std::string& initialPath    // Start folder
) {
    static std::string selectedFile;

    // Layout: Button on left, path display on right
    ImGui::BeginGroup();
    if (ImGui::Button(buttonLabel.c_str())) {
        ImGuiFileDialog::Instance()->OpenDialog(dialogKey.c_str(), "Select File", fileFilters.c_str());
    }
    ImGui::SameLine();

    if (!selectedFile.empty()) {
        ImGui::TextWrapped("%s", selectedFile.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No file selected");
    }
    ImGui::EndGroup();

    // Display the dialog
    if (ImGuiFileDialog::Instance()->Display(dialogKey.c_str(), ImGuiWindowFlags_NoCollapse, ImVec2(600, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
            ImGuiFileDialog::Instance()->Close();
            return selectedFile;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    return std::nullopt;
}