#include "menu.hpp"

namespace menu {
    void apply_style() {
        ImGuiStyle& style = ImGui::GetStyle();
        
        // Dark Fatality-inspired theme
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        
        // Rounding and padding
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.WindowPadding = ImVec2(8, 8);
        style.FramePadding = ImVec2(4, 4);
        
        // ... additional style settings
    }
}