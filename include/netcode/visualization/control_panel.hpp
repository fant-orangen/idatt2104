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

private:
    Rectangle bounds_;
    int selectedTab_;
    
    // Placeholder data for UI elements
    char textBuffer_[256];
    int dropdownIndex_;
    bool toggleState_;
    float sliderValue_;
    
    void renderMainTab();
    void renderPlayer1Tab();
    void renderServerTab();
    void renderPlayer2Tab();
};

}} // namespace netcode::visualization
