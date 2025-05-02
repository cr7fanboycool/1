#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

inline ID3D11Device* g_pd3dDevice = nullptr;
inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
inline HWND g_hWnd = nullptr;

void initialize_hooks();
void shutdown_hooks();
