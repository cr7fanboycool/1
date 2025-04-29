#pragma once
#include <cstdint>
#include <d3d11.h>

namespace interfaces {
    inline void* client = nullptr;
    inline void* engine = nullptr;
    inline void* entity_list = nullptr;
    inline ID3D11Device* dx11_device = nullptr;
    inline ID3D11DeviceContext* dx11_context = nullptr;

    bool initialize() {
        // Implement actual interface grabbing
        return client && engine && entity_list;
    }
}