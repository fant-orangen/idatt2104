#pragma once

#include <raylib.h>

namespace netcode {
namespace visualization {

/**
 * @brief The main control panel widget
 *
 * This class represents the main control panel widget. It handles rendering and
 * input processing for the control panel.
 */
class ControlPanel {
public:
    /**
     * @brief Construct a new ControlPanel object
     *
     * @param x The x-coordinate of the top-left corner
     * @param y The y-coordinate of the top-left corner
     * @param width The width of the control panel
     * @param height The height of the control panel
     */
    ControlPanel(float x, float y, float width, float height);
    /**
     * @brief Render the control panel
     */
    void render();
    /**
     * @brief Handle mouse interaction
     *
     * @param mousePos The current mouse position
     * @return true if any control was clicked, false otherwise
     */
    bool handleMouseInteraction(Vector2 mousePos);
    /**
     * @brief Handle keyboard input
     */
    void handleInput();
    
    /**
     * @brief Check if any text field is being edited
     *
     * @return true if any text field is in edit mode, false otherwise
     */
    bool isTextFieldActive() const { return textFieldActive_; }

private:
    Rectangle bounds_;
    int selectedTab_;
    
    // UI elements
    char textBuffer_[256];
    int dropdownIndex_;
    bool toggleState_;
    float sliderValue_;
    
    // Network delay sliders
    float clientToServerDelay_ = 10.0f;  // Default to 10ms
    float serverToClientDelay_ = 500.0f; // Default to 500ms
    
    // Track if any text field is in edit mode
    bool textFieldActive_ = false;
    
public:
    // Getter methods for network delays
    float getClientToServerDelay() const { return clientToServerDelay_; }
    float getServerToClientDelay() const { return serverToClientDelay_; }
    
    // Render functions for each tab
    void renderMainTab();
    void renderPlayer1Tab();
    void renderServerTab();
    void renderPlayer2Tab();
};

}} // namespace netcode::visualization
