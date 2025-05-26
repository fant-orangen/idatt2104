#pragma once

#include <raylib.h>

namespace netcode {
namespace visualization {

// Forward declaration
class ConcreteSettings;

// Helper struct to hold player control text fields
struct PlayerControls {
    char* forwardText;
    char* backwardText;
    char* leftText;
    char* rightText;
    bool* forwardActive;
    bool* backwardActive;
    bool* leftActive;
    bool* rightActive;
};

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
     * @param settings Reference to the concrete settings (optional, can be set later)
     */
    ControlPanel(float x, float y, float width, float height, ConcreteSettings* settings = nullptr);
    
    /**
     * @brief Set the settings reference
     * 
     * @param settings Pointer to the concrete settings
     */
    void setSettings(ConcreteSettings* settings);
    
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
    
    // Settings reference
    ConcreteSettings* settings_;
    
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
    
    // Text fields for player movement directions
    char player1ForwardText_[256] = "";
    char player1BackwardText_[256] = "";
    char player1LeftText_[256] = "";
    char player1RightText_[256] = "";
    
    char player2ForwardText_[256] = "";
    char player2BackwardText_[256] = "";
    char player2LeftText_[256] = "";
    char player2RightText_[256] = "";
    
    // Replace existing player text field active states
    bool player1ForwardActive_ = false;
    bool player1BackwardActive_ = false;
    bool player1LeftActive_ = false;
    bool player1RightActive_ = false;
    
    bool player2ForwardActive_ = false;
    bool player2BackwardActive_ = false;
    bool player2LeftActive_ = false;
    bool player2RightActive_ = false;
    
public:
    // Getter methods for network delays
    float getClientToServerDelay() const { return clientToServerDelay_; }
    float getServerToClientDelay() const { return serverToClientDelay_; }
    
    /**
     * @brief Check if any text field (including player fields) is active
     * @return true if any text field is being edited, false otherwise
     */
    bool isAnyTextFieldActive() const {
        return textFieldActive_ || 
               player1ForwardActive_ || player1BackwardActive_ || 
               player1LeftActive_ || player1RightActive_ ||
               player2ForwardActive_ || player2BackwardActive_ ||
               player2LeftActive_ || player2RightActive_;
    }
    
private:
    // Render functions for each tab
    void renderMainTab();
    void renderPlayer1Tab();
    void renderPlayer2Tab();
    
    // Helper functions for rendering and saving player settings
    void renderPlayerTab(int playerNum, const PlayerControls& controls);
    void savePlayerSettings(int playerNum);
    
    // Save settings functions (now just wrappers around savePlayerSettings)
    void savePlayer1Settings();
    void savePlayer2Settings();
};

}} // namespace netcode::visualization
