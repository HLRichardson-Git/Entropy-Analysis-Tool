cmake_minimum_required(VERSION 3.10)
project(EntropyAnalysisTool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)


include(FetchContent)

# Fetch Lib90B
set(Lib90B_BUILD_TESTS OFF CACHE BOOL "Build unit tests" FORCE)

FetchContent_Declare(
    lib90b
    URL https://github.com/HLRichardson-Git/Lib90B/archive/refs/tags/v0.2.zip
)

FetchContent_MakeAvailable(lib90b)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party
    third_party/imgui
    third_party/imgui/backends
    third_party/implot
    third_party/nlohmann
    third_party/ImGuiFileDialog
)

set (SOURCES
    main.cpp
    src/gui_platform/gui_platform.cpp
    src/core/application.cpp
    src/core/app_command/app_command.cpp
    src/data/data_manager.cpp
    src/data/histogram/histogram.cpp
    src/ui/ui_manager.cpp
    src/ui/heuristic_assessment/heuristic_manager.cpp
    src/file_utils/file_utils.cpp
)

file(GLOB IMGUI_SOURCES
    "third_party/imgui/*.cpp"
    "third_party/imgui/backends/*.cpp"
)

file(GLOB IMPLOT_SOURCES
    "third_party/implot/*.cpp"
)

file(GLOB NLOHMANN_JSON_SOURCES
    "third_party/nlohmann/*.hpp"
)

file(GLOB IMGUIFILEDIALOG_SOURCES
    "third_party/ImGuiFileDialog/*.cpp"
)

add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${IMGUI_SOURCES} 
    ${IMPLOT_SOURCES} 
    ${NLOHMANN_JSON_SOURCES}
    ${IMGUIFILEDIALOG_SOURCES}
    assets/resources.rc
) 

target_link_libraries(${PROJECT_NAME} 
PRIVATE 
    d3d11 
    d3dcompiler 
    dxguid 
    dwmapi
    Lib90B
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${lib90b_SOURCE_DIR}/include
        ${lib90b_SOURCE_DIR}/util
)