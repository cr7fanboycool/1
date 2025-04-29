#include "hooks.hpp"
#include "menu.hpp"
#include "interfaces.hpp"
#include <thread>
#include <chrono>

std::atomic<bool> g_unloading = false;

void cheat_thread(HMODULE module) noexcept {
    try {
        #ifdef _DEBUG
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        #endif

        while (!GetModuleHandleA("serverbrowser.dll"))
            std::this_thread::sleep_for(100ms);

        if (!interfaces::initialize()) {
            MessageBoxA(nullptr, "Failed to get interfaces", "Error", MB_ICONERROR);
            goto cleanup;
        }

        if (!hooks::initialize()) {
            MessageBoxA(nullptr, "Failed to initialize hooks", "Error", MB_ICONERROR);
            goto cleanup;
        }

        while (!g_unloading) {
            if (GetAsyncKeyState(VK_END))
                g_unloading = true;
            std::this_thread::sleep_for(50ms);
        }

    cleanup:
        hooks::shutdown();
        menu::shutdown();
        #ifdef _DEBUG
        FreeConsole();
        #endif
        FreeLibraryAndExitThread(module, 0);
    } catch (...) {
        MessageBoxA(nullptr, "Unhandled exception", "Error", MB_ICONERROR);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)cheat_thread, module, 0, nullptr);
    }
    return TRUE;
}