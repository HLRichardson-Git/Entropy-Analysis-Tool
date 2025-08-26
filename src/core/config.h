
#pragma once

#include "types.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace Config {
    inline ImFont* fontH1 = nullptr;
    inline ImFont* fontH2 = nullptr;
    inline ImFont* fontH3 = nullptr;
    inline ImFont* normal = nullptr;
    inline ImFont* icons = nullptr;

    // Application info
    constexpr const char* APPLICATION_TITLE = "Entropy Analysis Tool";
    constexpr const char* APPLICATION_VERSION = "0.1.0";
    
    // Window settings
    constexpr int DEFAULT_WINDOW_WIDTH = 1280;
    constexpr int DEFAULT_WINDOW_HEIGHT = 800;
    
    // Font settings
    constexpr float DEFAULT_FONT_SIZE = 16.0f;
    constexpr float HEADER_FONT_SIZE = 18.0f;
    constexpr float TITLE_FONT_SIZE = 24.0f;
    
    // UI settings
    constexpr float MENU_BAR_HEIGHT = 25.0f;
    constexpr float HEADER_HEIGHT = 60.0f;
    constexpr float DASHBOARD_HEIGHT = 120.0f;
    
    // Dark theme colors
    constexpr float BACKGROUND_COLOR[4] = {0.15f, 0.15f, 0.15f, 1.00f};     // Dark grey background
    constexpr float PRIMARY_COLOR[4] = {0.25f, 0.58f, 0.98f, 1.00f};        // Blue accent
    constexpr float SUCCESS_COLOR[4] = {0.2f, 0.7f, 0.2f, 1.0f};            // Green
    constexpr float WARNING_COLOR[4] = {0.9f, 0.6f, 0.1f, 1.0f};            // Orange
    constexpr float ERROR_COLOR[4] = {0.9f, 0.3f, 0.3f, 1.0f};              // Red
    constexpr float TEXT_COLOR[4] = {0.90f, 0.90f, 0.90f, 1.00f};           // Light grey text
    constexpr float CARD_BG_COLOR[4] = {0.18f, 0.18f, 0.18f, 1.00f};        // Card background

    struct AppConfig {
        const char* APPLICATION_TITLE = "Entropy Analysis Tool";
        const char* APPLICATION_VERSION = "0.1.0";
        Project lastOpenedProject;
        std::vector<Project> savedProjects;
    };
}