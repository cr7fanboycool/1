#include "hooks.hpp"

// Setup all necessary hooks for the HVH internal functions.
void SetupHooks() {
    // Hook the present function to overlay ImGui UI
    HookPresent();
    // Other function hooks specific to game actions (like Aim, Move, etc.)
}

// The hooked presentation method for rendering overlays
void __fastcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    // Call original Present function to avoid breaking game functionality
    OriginalPresent(pSwapChain, SyncInterval, Flags);
    
    // Render ImGui overlay for user interface
    RenderImGui();
}