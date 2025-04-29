// In your Present hook:
void __stdcall hkPresent(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
    static bool init = false;
    if (!init) {
        ID3D11Device* device;
        ID3D11DeviceContext* context;
        swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&device);
        device->GetImmediateContext(&context);
        
        menu::initialize(device, context);
        init = true;
    }

    // Handle input and render
    menu::handle_input();
    
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    menu::render();
    
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}