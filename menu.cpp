void render_hvh_tab() {
    ImGui::BeginChild("HVH", ImVec2(0, 0), true);
    {
        ImGui::Checkbox("Enable Anti-Aim", &hvh::anti_aim);
        ImGui::Combo("Pitch", &hvh::pitch_mode, "None\0Down\0Up\0");
        ImGui::Combo("Yaw", &hvh::yaw_mode, "Static\0Jitter\0Spin\0");
        ImGui::SliderFloat("Yaw Offset", &hvh::yaw_offset, -180.f, 180.f, "%.1f°");
        
        ImGui::Separator();
        
        ImGui::Checkbox("Fake Duck", &hvh::fake_duck);
        ImGui::Checkbox("Fake Lag", &hvh::fakelag);
        if (hvh::fakelag) {
            ImGui::Combo("Mode", &hvh::fakelag_mode, "Static\0Dynamic\0");
            ImGui::SliderInt("Amount", &hvh::fakelag_amount, 1, 14);
        }
        
        ImGui::Separator();
        
        ImGui::Checkbox("Resolver", &hvh::resolver);
        if (hvh::resolver) {
            ImGui::Checkbox("Force Safepoint", &hvh::force_safepoint);
        }
    }
    ImGui::EndChild();
}