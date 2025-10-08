
#include "file_utils.h"

void from_json(const json& j, Project& p) {
    j.at("vendor").get_to(p.vendor);
    j.at("repo").get_to(p.repo);
    j.at("projectName").get_to(p.name);
    j.at("projectPath").get_to(p.path);
}

void to_json(json& j, const Project& p) {
    j = json{
        {"vendor", p.vendor},
        {"repo", p.repo},
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
    ImGui::PushFont(Config::fontH3);
    ImGui::PushStyleColor(ImGuiCol_Button,        Config::GREY_BUTTON.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Config::GREY_BUTTON.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Config::GREY_BUTTON.active);
    ImGui::PushStyleColor(ImGuiCol_Text, Config::TEXT_DARK_CHARCOAL);
    {
        std::string buttonLabelWithIcon = std::string(reinterpret_cast<const char*>(u8"\uf574")) + " " + buttonLabel;
        if (ImGui::Button(buttonLabelWithIcon.c_str())) {
            ImGuiFileDialog::Instance()->OpenDialog(dialogKey.c_str(), "Select File", fileFilters.c_str());
        }
    }
    ImGui::PopStyleColor(4);
    ImGui::PopFont();
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

std::string toWslCommandPath(const std::filesystem::path& winPath) {
    std::string path = winPath.string();
    std::replace(path.begin(), path.end(), '\\', '/');

    // handle drive letter
    if (path.size() > 2 && path[1] == ':') {
        char driveLetter = std::tolower(path[0]);
        path = "/mnt/" + std::string(1, driveLetter) + path.substr(2);
    }

    // wrap in quotes for shell
    return "\"" + path + "\"";
}

std::string executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

void writeStringToFile(const std::string& content, const std::filesystem::path& filePath) {
    // Ensure the parent directory exists
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filePath.string());
    }

    outFile << content;

    if (!outFile.good()) {
        throw std::runtime_error("Failed to write data to file: " + filePath.string());
    }

    outFile.close();
}