#pragma once

#include <raylib.h>

namespace netcode {
namespace visualization {

class ControlPanel {
public:
    ControlPanel(float x, float y, float width, float height);
    void render();
    bool handleMouseInteraction(Vector2 mousePos);
    void handleInput();
    
    // Check if any text field is being edited
    bool isTextFieldActive() const { return textFieldActive_; }

private:
    Rectangle bounds_;
    int selectedTab_;
    
    // Placeholder data for UI elements
    char textBuffer_[256];
    int dropdownIndex_;
    bool toggleState_;
    float sliderValue_;
    
    // Track if any text field is in edit mode
    bool textFieldActive_ = false;
    
    void renderMainTab();
    void renderPlayer1Tab();
    void renderServerTab();
    void renderPlayer2Tab();
};

}} // namespace netcode::visualization
