
#pragma once

#include <windows.h>

// Helper Functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

bool InitPlatformBackend(HWND& hwnd);
void ShutdownPlatformBackend();
void StartImGuiFrame();
void PresentFrame();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
