#include "netcode/visualization/control_panel.hpp"

// Define RAYGUI_IMPLEMENTATION before including raygui.h
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "netcode/visualization/raygui.h"

namespace netcode {
namespace visualization {

ControlPanel::ControlPanel(float x, float y, float width, float height)
    : bounds_({x, y, width, height}), 
      selectedTab_(0),
      dropdownIndex_(0),
      toggleState_(false),
      sliderValue_(50.0f) {
    strcpy(textBuffer_, "Sample text");
}

bool ControlPanel::handleMouseInteraction(Vector2 mousePos) {
    return CheckCollisionPointRec(mousePos, bounds_);
}

void ControlPanel::handleInput() {
    // No input handling needed for this minimal implementation
}

void ControlPanel::renderMainTab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;  // Below tabs
    float spacing = 30;
    
    // Network delay sliders section
    GuiLabel((Rectangle){startX, startY, 200.0f, 20.0f}, "Network Delays (ms)");
    
    // Client to Server delay slider with label
    GuiLabel((Rectangle){startX, startY + spacing - 15, 200.0f, 20.0f}, "Client -> Server Delay");
    GuiSlider((Rectangle){startX, startY + spacing, 200.0f, 20.0f}, 
              "",  // Empty label since we have a separate label above
              TextFormat("%.0f", clientToServerDelay_), 
              &clientToServerDelay_, 0, 500);
    
    // Server to Client delay slider with label
    GuiLabel((Rectangle){startX, startY + spacing * 2 - 15, 200.0f, 20.0f}, "Server -> Client Delay");
    GuiSlider((Rectangle){startX, startY + spacing * 2, 200.0f, 20.0f}, 
              "",  // Empty label since we have a separate label above
              TextFormat("%.0f", serverToClientDelay_), 
              &serverToClientDelay_, 0, 500);
    
    // Sample elements (kept for reference, can be removed if not needed)
    /*
    // Sample button
    if (GuiButton((Rectangle){startX + 120, startY, 100, 20}, "Click Me")) {
        // Button click handling would go here
    }
    
    // Sample dropdown
    const char* items = "Option 1;Option 2;Option 3";
    GuiComboBox((Rectangle){startX, startY + spacing * 3, 150, 20}, items, &dropdownIndex_);
    
    // Sample text field - check if it's being edited
    int result = GuiTextBox((Rectangle){startX + 170, startY + spacing * 3, 150, 20}, textBuffer_, 256, textFieldActive_);
    if (result == 1) textFieldActive_ = !textFieldActive_; // Toggle edit mode
    
    // Sample toggle
    GuiCheckBox((Rectangle){startX, startY + spacing * 4, 20, 20}, "Toggle Option", &toggleState_);
    */
}

void ControlPanel::renderPlayer1Tab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 1 Controls");
    GuiButton((Rectangle){startX, startY + 30, 150, 20}, "Reset Player 1");
    GuiCheckBox((Rectangle){startX, startY + 60, 20, 20}, "Player 1 Active", &toggleState_);
}

void ControlPanel::renderServerTab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    float spacing = 30;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Server Settings");
    
    // Server address text field - check if it's being edited
    static char serverAddress[16] = "127.0.0.1";
    int result = GuiTextBox((Rectangle){startX, startY + spacing, 150, 20}, serverAddress, 16, textFieldActive_);
    if (result == 1) textFieldActive_ = !textFieldActive_; // Toggle edit mode
    
    // Connect button
    if (GuiButton((Rectangle){startX, startY + spacing * 2, 150, 20}, "Connect")) {
        // Connection logic would go here
    }
}

void ControlPanel::renderPlayer2Tab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 2 Controls");
    GuiButton((Rectangle){startX, startY + 30, 150, 20}, "Reset Player 2");
    GuiCheckBox((Rectangle){startX, startY + 60, 20, 20}, "Player 2 Active", &toggleState_);
}

void ControlPanel::render() {
    // Draw panel background
    DrawRectangle(bounds_.x, bounds_.y, bounds_.width, bounds_.height, LIGHTGRAY);
    DrawRectangleLines(bounds_.x, bounds_.y, bounds_.width, bounds_.height, DARKGRAY);
    
    // Draw tabs
    Rectangle tabRect1 = {bounds_.x + 10, bounds_.y + 5, 100, 30};
    Rectangle tabRect2 = {bounds_.x + 120, bounds_.y + 5, 100, 30};
    Rectangle tabRect3 = {bounds_.x + 230, bounds_.y + 5, 100, 30};
    Rectangle tabRect4 = {bounds_.x + 340, bounds_.y + 5, 100, 30};
    
    if (GuiButton(tabRect1, "Main")) selectedTab_ = 0;
    if (GuiButton(tabRect2, "Player 1")) selectedTab_ = 1;
    if (GuiButton(tabRect3, "Server")) selectedTab_ = 2;
    if (GuiButton(tabRect4, "Player 2")) selectedTab_ = 3;
    
    // Render content based on selected tab
    switch (selectedTab_) {
        case 0: renderMainTab(); break;
        case 1: renderPlayer1Tab(); break;
        case 2: renderServerTab(); break;
        case 3: renderPlayer2Tab(); break;
    }
}

}} // namespace netcode::visualization
