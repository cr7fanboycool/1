// menu.cpp - Implementation of the main menu for the cheat user interface
#include "menu.hpp"

// Function to setup menu items and layout
void SetupMenu() {
    // Create menu structure using ImGui for toggling features
    ImGui::Begin("HVH Menu"); // Start the ImGui window
    // Add various toggles for features like Aimbot, Triggerbot, etc.
    ImGui::End();
}

// Render the ImGui menu each frame to allow user interaction
void RenderMenu() {
    SetupMenu(); // Prepare the menu items
}