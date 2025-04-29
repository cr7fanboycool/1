#include "hooks.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace hooks {
    inline WNDPROC original_wndproc = nullptr;
    
    LRESULT __stdcall wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        // Block input when menu is open
        if (menu::is_open) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
            
            // Block game input when interacting with UI
            if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) {
                switch (msg) {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MOUSEWHEEL:
                    return true;
                }
            }
        }
        
        return CallWindowProc(original_wndproc, hwnd, msg, wparam, lparam);
    }
}