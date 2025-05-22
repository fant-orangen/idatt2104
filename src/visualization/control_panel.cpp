#include "netcode/visualization/control_panel.hpp"

// Define RAYGUI_IMPLEMENTATION before including raygui.h
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "netcode/visualization/raygui.h"
#include "netcode/visualization/settings.hpp"
#include <regex>

namespace netcode {
namespace visualization {

// Helper function to convert key codes to display characters
const char* keyToChar(KeyboardKey key) {
    static char display[2] = {0};
    switch(key) {
        case KEY_UP: return "↑";
        case KEY_DOWN: return "↓";
        case KEY_LEFT: return "←";
        case KEY_RIGHT: return "→";
        default: 
            display[0] = (key >= 32 && key <= 126) ? key : '?';
            return display;
    }
}

// Helper function to convert char to key code
KeyboardKey charToKey(char c) {
    c = toupper(c);
    if (c >= 'A' && c <= 'Z') return static_cast<KeyboardKey>(c);
    return KEY_NULL;
}

ControlPanel::ControlPanel(float x, float y, float width, float height)
    : bounds_({x, y, width, height}), 
      selectedTab_(0),
      dropdownIndex_(0),
      toggleState_(false),
      sliderValue_(50.0f) {
    strcpy(textBuffer_, "Sample text");
    
    // Initialize with display characters
    strncpy(player1ForwardText_, keyToChar(settings::PLAYER1_UP), 1);
    strncpy(player1BackwardText_, keyToChar(settings::PLAYER1_DOWN), 1);
    strncpy(player1LeftText_, keyToChar(settings::PLAYER1_LEFT), 1);
    strncpy(player1RightText_, keyToChar(settings::PLAYER1_RIGHT), 1);
    
    strncpy(player2ForwardText_, keyToChar(settings::PLAYER2_UP), 1);
    strncpy(player2BackwardText_, keyToChar(settings::PLAYER2_DOWN), 1);
    strncpy(player2LeftText_, keyToChar(settings::PLAYER2_LEFT), 1);
    strncpy(player2RightText_, keyToChar(settings::PLAYER2_RIGHT), 1);
}

bool ControlPanel::handleMouseInteraction(Vector2 mousePos) {
    return CheckCollisionPointRec(mousePos, bounds_);
}

void ControlPanel::handleInput() {
    // No input handling needed for this minimal implementation
}

// Helper function to validate and enforce single character input
void validateSingleCharInput(char* text) {
    // If more than one character, keep only the first one
    if (text[0] != '\0' && text[1] != '\0') {
        text[1] = '\0';
    }
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
}

void ControlPanel::renderPlayer1Tab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    float spacing = 30;
    float textFieldWidth = 100;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 1 Controls");
    GuiButton((Rectangle){startX, startY + spacing, 150, 20}, "Reset Player 1");
    GuiCheckBox((Rectangle){startX, startY + spacing * 2, 20, 20}, "Player 1 Active", &toggleState_);
    
    // Forward text field
    GuiLabel((Rectangle){startX, startY + spacing * 3, textFieldWidth, 20}, "Forward");
    if (GuiTextBox((Rectangle){startX, startY + spacing * 4, textFieldWidth, 20}, player1ForwardText_, 256, player1ForwardActive_)) {
        player1ForwardActive_ = !player1ForwardActive_;
    }
    validateSingleCharInput(player1ForwardText_);
    
    // Backward text field
    GuiLabel((Rectangle){startX + textFieldWidth + 10, startY + spacing * 3, textFieldWidth, 20}, "Backward");
    if (GuiTextBox((Rectangle){startX + textFieldWidth + 10, startY + spacing * 4, textFieldWidth, 20}, player1BackwardText_, 256, player1BackwardActive_)) {
        player1BackwardActive_ = !player1BackwardActive_;
    }
    validateSingleCharInput(player1BackwardText_);
    
    // Left text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 3, textFieldWidth, 20}, "Left");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 4, textFieldWidth, 20}, player1LeftText_, 256, player1LeftActive_)) {
        player1LeftActive_ = !player1LeftActive_;
    }
    validateSingleCharInput(player1LeftText_);
    
    // Right text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 3, textFieldWidth, 20}, "Right");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 4, textFieldWidth, 20}, player1RightText_, 256, player1RightActive_)) {
        player1RightActive_ = !player1RightActive_;
    }
    validateSingleCharInput(player1RightText_);
    
    // Save button - placed to the right of the text fields
    if (GuiButton((Rectangle){startX + (textFieldWidth + 10) * 4, startY + spacing * 4, 100, 20}, "Save Changes")) {
        savePlayer1Settings();
    }
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
    float spacing = 30;
    float textFieldWidth = 100;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 2 Controls");
    GuiButton((Rectangle){startX, startY + spacing, 150, 20}, "Reset Player 2");
    GuiCheckBox((Rectangle){startX, startY + spacing * 2, 20, 20}, "Player 2 Active", &toggleState_);
    
    // Forward text field
    GuiLabel((Rectangle){startX, startY + spacing * 3, textFieldWidth, 20}, "Forward");
    if (GuiTextBox((Rectangle){startX, startY + spacing * 4, textFieldWidth, 20}, player2ForwardText_, 256, player2ForwardActive_)) {
        player2ForwardActive_ = !player2ForwardActive_;
    }
    validateSingleCharInput(player2ForwardText_);
    
    // Backward text field
    GuiLabel((Rectangle){startX + textFieldWidth + 10, startY + spacing * 3, textFieldWidth, 20}, "Backward");
    if (GuiTextBox((Rectangle){startX + textFieldWidth + 10, startY + spacing * 4, textFieldWidth, 20}, player2BackwardText_, 256, player2BackwardActive_)) {
        player2BackwardActive_ = !player2BackwardActive_;
    }
    validateSingleCharInput(player2BackwardText_);
    
    // Left text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 3, textFieldWidth, 20}, "Left");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 4, textFieldWidth, 20}, player2LeftText_, 256, player2LeftActive_)) {
        player2LeftActive_ = !player2LeftActive_;
    }
    validateSingleCharInput(player2LeftText_);
    
    // Right text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 3, textFieldWidth, 20}, "Right");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 4, textFieldWidth, 20}, player2RightText_, 256, player2RightActive_)) {
        player2RightActive_ = !player2RightActive_;
    }
    validateSingleCharInput(player2RightText_);
    
    // Save button - placed to the right of the text fields
    if (GuiButton((Rectangle){startX + (textFieldWidth + 10) * 4, startY + spacing * 4, 100, 20}, "Save Changes")) {
        savePlayer2Settings();
    }
}

void ControlPanel::savePlayer1Settings() {
    if (player1ForwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1ForwardText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER1_UP = newKey;
    }
    if (player1BackwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1BackwardText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER1_DOWN = newKey;
    }
    if (player1LeftText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1LeftText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER1_LEFT = newKey;
    }
    if (player1RightText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1RightText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER1_RIGHT = newKey;
    }
}

void ControlPanel::savePlayer2Settings() {
    if (player2ForwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2ForwardText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER2_UP = newKey;
    }
    if (player2BackwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2BackwardText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER2_DOWN = newKey;
    }
    if (player2LeftText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2LeftText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER2_LEFT = newKey;
    }
    if (player2RightText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2RightText_[0]);
        if (newKey != KEY_NULL) settings::PLAYER2_RIGHT = newKey;
    }
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
