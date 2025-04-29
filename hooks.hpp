#pragma once
#include <MinHook.h>
#include <Windows.h>
#include <cstdint>

// Error handling macros
#define HOOK_CHECK(expr) if (MH_Status status = (expr); status != MH_OK) { \
    LogError(__LINE__, __FUNCTION__, status); return false; }

namespace hooks {
    // Hook status tracking
    enum class HookStatus : uint8_t {
        NOT_CREATED,
        ENABLED,
        DISABLED,
        DESTROYED
    };

    // Base hook wrapper
    template <typename T>
    struct Hook {
        T original = nullptr;
        HookStatus status = HookStatus::NOT_CREATED;
        const char* name = "unnamed_hook";
    };

    // Specific hook declarations
    namespace functions {
        // CreateMove hook
        using CreateMove_t = bool(__fastcall*)(void*, void*, float, void*);
        inline Hook<CreateMove_t> CreateMove;
        
        // Add other hooks here following the same pattern
        // Example:
        // using PaintTraverse_t = void(__thiscall*)(void*, void*, int, bool, bool);
        // inline Hook<PaintTraverse_t> PaintTraverse;
    }

    // Initialization
    bool initialize() noexcept;
    void shutdown() noexcept;
    
    // Hook management
    template <typename T>
    bool create_hook(Hook<T>& hook, void* target, T detour, const char* name = nullptr) noexcept;
    
    bool enable_all() noexcept;
    bool disable_all() noexcept;

    // Utility functions
    void LogError(int line, const char* function, MH_STATUS status) noexcept;
    void* find_pattern(const char* module, const char* pattern) noexcept;
}

// Template implementations
namespace hooks {
    template <typename T>
    bool create_hook(Hook<T>& hook, void* target, T detour, const char* name) noexcept {
        if (hook.status != HookStatus::NOT_CREATED) {
            return false;
        }

        HOOK_CHECK(MH_CreateHook(target, reinterpret_cast<void*>(detour), 
            reinterpret_cast<void**>(&hook.original)));
        
        hook.status = HookStatus::DISABLED;
        hook.name = name ? name : "unnamed_hook";
        return true;
    }
}