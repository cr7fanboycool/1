#include <Windows.h>
#include <d3d11.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "menu.hpp"  // for render_hvh_tab() and menu_open

// Define global toggle flag
bool menu_open = false;

// Your device/context from your hook
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern HWND g_hWnd;

bool initialized = false;

void InitImGui() {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    initialized = true;
}

void RenderMenu() {
    if (GetAsyncKeyState(VK_INSERT) & 1)
        menu_open = !menu_open;

    if (!initialized)
        InitImGui();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (menu_open) {
        ImGui::Begin("HVH Menu", &menu_open, ImGuiWindowFlags_AlwaysAutoResize);
        render_hvh_tab();
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
