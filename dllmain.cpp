#include <windows.h>

/// Entry point for the DLL.
/// This function is called when the DLL is loaded or unloaded.
/// It sets up hooks for relevant game functions and initializes
/// necessary components.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // Initialize the hack components when the process attaches
            InitializeHooks();
            break;
        case DLL_PROCESS_DETACH:
            // Clean-up hooks and resources when the process detach occurs
            CleanupHooks();
            break;
    }
    return TRUE; // Return success
}