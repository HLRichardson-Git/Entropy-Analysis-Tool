#include <iostream>
#include <memory>
#include <stdexcept>

#include "imgui.h"
#include "src/gui_platform/gui_platform.h"
#include "src/core/application.h"

// Global application instance
//static std::unique_ptr<Application> g_app = nullptr;
static Application app;
static HWND g_hwnd = nullptr;

int main() {
    try {
        // Initialize platform backend (your existing DX11 setup)
        if (!InitPlatformBackend(g_hwnd)) {
            std::cerr << "Failed to initialize platform backend" << std::endl;
            return -1;
        }
        
        // Create and initialize application
        if (!app.Initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            ShutdownPlatformBackend();
            return -1;
        }
        
        // Main application loop
        bool done = false;
        while (!done) {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                
                if (msg.message == WM_QUIT)
                    done = true;
            }
            
            if (done)
                break;
            
            // Start ImGui frame
            StartImGuiFrame();
            
            // Update and render application
            try {
                app.Update();
                app.Render();
            } catch (const std::exception& e) {
                std::cerr << "Exception in application loop: " << e.what() << std::endl;
            }
            
            // Present frame
            PresentFrame();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;

            app.Shutdown();

        ShutdownPlatformBackend();
        return -1;
    }
    
    // Cleanup
    app.Shutdown();
    ShutdownPlatformBackend();
    
    return 0;
}