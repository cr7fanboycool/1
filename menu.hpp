bool menu_open = true; // global

void RenderMenu() {
    if (GetAsyncKeyState(VK_INSERT) & 1)
        menu_open = !menu_open;

    if (!menu_open)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("HVH Menu", &menu_open, ImGuiWindowFlags_AlwaysAutoResize);
    render_hvh_tab(menu.cpp); // your HVH tab from menu.cpp
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
