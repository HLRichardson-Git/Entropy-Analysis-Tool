
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