#ifndef HOOKS_HPP
#define HOOKS_HPP

// Function to setup all hooks for the internal cheat
void SetupHooks( )

// Hooked present function to manage rendering
void __fastcall HookedPresent(IDXGISwapChain*, UINT, UINT);

#endif // HOOKS_HPP