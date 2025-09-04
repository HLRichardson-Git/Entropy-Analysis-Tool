
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
    const std::string& dialogKey,
    const std::string& buttonLabel,
    const std::string& fileFilters,
    const std::string& initialPath
) {
    ImGui::BeginGroup();
    if (ImGui::Button(buttonLabel.c_str())) {
        ImGuiFileDialog::Instance()->OpenDialog(dialogKey.c_str(), "Select File", fileFilters.c_str());
    }
    ImGui::EndGroup();

    if (ImGuiFileDialog::Instance()->Display(dialogKey.c_str(), ImGuiWindowFlags_NoCollapse, ImVec2(600, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            ImGuiFileDialog::Instance()->Close();
            return filePath;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    return std::nullopt;
}

std::optional<fs::path> CopyFileToDirectory(const fs::path& sourcePath, const fs::path& destDir) {
    try {
        if (!fs::exists(sourcePath)) {
            std::cerr << "Source file does not exist: " << sourcePath << "\n";
            return std::nullopt;
        }

        fs::create_directories(destDir);

        fs::path destPath = destDir / sourcePath.filename();
        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);

        return destPath;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error copying file: " << e.what() << "\n";
        return std::nullopt;
    }
}