#include "hooks.hpp"
#include "menu.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>
#include <MinHook.h>
#include "menu.hpp"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
PresentFn oPresent = nullptr;

HRESULT __stdcall hooked_present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool initialized = false;

    if (!initialized) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWnd = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();

            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

            initialized = true;
        }
    }

    if (initialized) {
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        RenderMenu(); // from menu.cpp
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void initialize_hooks() {
    if (MH_Initialize() != MH_OK)
        return;

    // Replace this with actual swapchain grabbing if needed
    // void** vtable = get_swapchain_vtable(); // You MUST get this yourself
    void** vtable = nullptr; // ← Replace this line

    oPresent = (PresentFn)vtable[8];

    if (MH_CreateHook((void*)oPresent, &hooked_present, (void**)&oPresent) == MH_OK)
        MH_EnableHook((void*)oPresent);
}

void shutdown_hooks() {
    MH_DisableHook((void*)oPresent);
    MH_RemoveHook((void*)oPresent);
    MH_Uninitialize();
}
