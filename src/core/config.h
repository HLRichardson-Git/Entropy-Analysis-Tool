
#pragma once

#include "types.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace Config {
    inline ImFont* fontH1 = nullptr;
    inline ImFont* fontH1_Bold = nullptr;
    inline ImFont* fontH2 = nullptr;
    inline ImFont* fontH2_Bold = nullptr;
    inline ImFont* fontH3 = nullptr;
    inline ImFont* fontH3_Bold = nullptr;
    inline ImFont* normal = nullptr;
    inline ImFont* icons = nullptr;

    // Application info
    constexpr const char* APPLICATION_TITLE = "Entropy Analysis Tool";
    constexpr const char* APPLICATION_VERSION = "2.0.0";
    
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
    const ImVec4 BACKGROUND_COLOR = {0.15f, 0.15f, 0.15f, 1.00f};     // Dark grey background
    const ImVec4 SLIGHTLY_LIGHTER_BACKGROUND_COLOR = {0.18f, 0.18f, 0.18f, 1.00f};
    const ImVec4 LIGHT_BACKGROUND_COLOR = {0.25f, 0.25f, 0.25f, 1.0f};
    constexpr float PRIMARY_COLOR[4] = {0.25f, 0.58f, 0.98f, 1.00f};        // Blue accent
    constexpr float SUCCESS_COLOR[4] = {0.2f, 0.7f, 0.2f, 1.0f};            // Green
    constexpr float WARNING_COLOR[4] = {0.9f, 0.6f, 0.1f, 1.0f};            // Orange
    constexpr float ERROR_COLOR[4] = {0.9f, 0.3f, 0.3f, 1.0f};              // Red
    constexpr float TEXT_COLOR[4] = {0.90f, 0.90f, 0.90f, 1.00f};           // Light grey text
    constexpr float CARD_BG_COLOR[4] = {0.18f, 0.18f, 0.18f, 1.00f};        // Card background

    // Font colors
    inline const ImVec4 TEXT_LIGHT_GREY     = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    inline const ImVec4 TEXT_MUTED_GREY     = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    inline const ImVec4 TEXT_DARK_CHARCOAL  = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    inline const ImVec4 TEXT_BLACK          = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);

    inline const ImVec4 TEXT_BLUE           = ImVec4(0.25f, 0.58f, 0.98f, 1.0f);
    inline const ImVec4 TEXT_GREEN          = ImVec4(0.20f, 0.80f, 0.20f, 1.0f);
    inline const ImVec4 TEXT_ORANGE         = ImVec4(0.95f, 0.75f, 0.25f, 1.0f);
    inline const ImVec4 TEXT_RED            = ImVec4(0.95f, 0.40f, 0.40f, 1.0f);
    inline const ImVec4 TEXT_CYAN           = ImVec4(0.35f, 0.75f, 0.95f, 1.0f);
    inline const ImVec4 TEXT_PURPLE         = ImVec4(0.70f, 0.60f, 1.0f, 1.0f);


    // ImGui theme colors
    struct ButtonPalette {
        ImVec4 normal;
        ImVec4 hovered;
        ImVec4 active;
    };

    inline const ButtonPalette GREEN_BUTTON = {
        {0.0f, 0.78f, 0.58f, 1.0f},
        {0.0f, 0.66f, 0.49f, 1.0f},
        {0.0f, 0.52f, 0.40f, 1.0f}
    };

    inline const ButtonPalette ORANGE_BUTTON = {
        {1.0f, 0.72f, 0.41f, 1.0f},
        {1.0f, 0.62f, 0.31f, 1.0f},
        {1.0f, 0.52f, 0.21f, 1.0f}
    };

    inline const ButtonPalette PURPLE_BUTTON = {
        {0.50f, 0.51f, 1.0f, 1.0f},
        {0.40f, 0.41f, 1.0f, 1.0f},
        {0.30f, 0.31f, 1.0f, 1.0f}
    };

    inline const ButtonPalette RED_BUTTON = {
        {0.86f, 0.52f, 0.45f, 1.0f},
        {0.76f, 0.42f, 0.35f, 1.0f},
        {0.66f, 0.32f, 0.25f, 1.0f}
    };

    inline const ButtonPalette BLUE_BUTTON = {
        {0.68f, 0.85f, 0.90f, 1.0f}, // normal
        {0.58f, 0.75f, 0.80f, 1.0f}, // hovered
        {0.48f, 0.65f, 0.70f, 1.0f}  // active
    };

    inline const ButtonPalette PINK_BUTTON = {
        {0.96f, 0.80f, 0.87f, 1.0f}, // normal
        {0.86f, 0.70f, 0.77f, 1.0f}, // hovered
        {0.76f, 0.60f, 0.67f, 1.0f}  // active
    };

    inline const ButtonPalette MINT_BUTTON = {
        {0.74f, 0.93f, 0.82f, 1.0f}, // normal
        {0.64f, 0.83f, 0.72f, 1.0f}, // hovered
        {0.54f, 0.73f, 0.62f, 1.0f}  // active
    };

    inline const ButtonPalette GREY_BUTTON = {
        {0.85f, 0.85f, 0.85f, 1.0f}, // normal
        {0.75f, 0.75f, 0.75f, 1.0f}, // hovered
        {0.65f, 0.65f, 0.65f, 1.0f}  // active
    };

    struct AppConfig {
        const char* APPLICATION_TITLE = "Entropy Analysis Tool";
        const char* APPLICATION_VERSION = "0.1.0";
        Project lastOpenedProject;
        std::vector<Project> savedProjects;
        std::vector<std::string> vendorsList;
    };
}