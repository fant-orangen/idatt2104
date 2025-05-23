#include "netcode/visualization/control_panel.hpp"

// Define RAYGUI_IMPLEMENTATION before including raygui.h
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "netcode/visualization/raygui.h"
#include "netcode/visualization/concrete_settings.hpp"
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

ControlPanel::ControlPanel(float x, float y, float width, float height, ConcreteSettings* settings)
    : bounds_({x, y, width, height}), 
      selectedTab_(0),
      settings_(settings),
      dropdownIndex_(0),
      toggleState_(false),
      sliderValue_(50.0f) {
    strcpy(textBuffer_, "Sample text");
    
    // Initialize with display characters from settings if available, otherwise use defaults
    if (settings_) {
        strncpy(player1ForwardText_, keyToChar(settings_->getPlayer1Up()), 1);
        strncpy(player1BackwardText_, keyToChar(settings_->getPlayer1Down()), 1);
        strncpy(player1LeftText_, keyToChar(settings_->getPlayer1Left()), 1);
        strncpy(player1RightText_, keyToChar(settings_->getPlayer1Right()), 1);
        
        strncpy(player2ForwardText_, keyToChar(settings_->getPlayer2Up()), 1);
        strncpy(player2BackwardText_, keyToChar(settings_->getPlayer2Down()), 1);
        strncpy(player2LeftText_, keyToChar(settings_->getPlayer2Left()), 1);
        strncpy(player2RightText_, keyToChar(settings_->getPlayer2Right()), 1);
    } else {
        // Use defaults
        strncpy(player1ForwardText_, "W", 1);
        strncpy(player1BackwardText_, "S", 1);
        strncpy(player1LeftText_, "A", 1);
        strncpy(player1RightText_, "D", 1);
        
        strncpy(player2ForwardText_, "I", 1);
        strncpy(player2BackwardText_, "K", 1);
        strncpy(player2LeftText_, "J", 1);
        strncpy(player2RightText_, "L", 1);
    }
}

void ControlPanel::setSettings(ConcreteSettings* settings) {
    settings_ = settings;
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

    // Add prediction and interpolation checkboxes
    if (settings_) {
        bool prediction = settings_->isPredictionEnabled();
        bool interpolation = settings_->isInterpolationEnabled();
        
        GuiLabel((Rectangle){startX, startY + spacing * 3, 200.0f, 20.0f}, "Game Settings");
        
        if (GuiCheckBox((Rectangle){startX, startY + spacing * 4, 20, 20}, "Enable Prediction", &prediction)) {
            settings_->setPredictionEnabled(prediction);
        }
        if (GuiCheckBox((Rectangle){startX + 180, startY + spacing * 4, 20, 20}, "Enable Interpolation", &interpolation)) {
            settings_->setInterpolationEnabled(interpolation);
        }
    }
}

void ControlPanel::renderPlayer1Tab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    float spacing = 30;
    float textFieldWidth = 100;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 1 Controls");
    
    // Forward text field
    GuiLabel((Rectangle){startX, startY + spacing, textFieldWidth, 20}, "Forward");
    if (GuiTextBox((Rectangle){startX, startY + spacing * 2, textFieldWidth, 20}, player1ForwardText_, 256, player1ForwardActive_)) {
        player1ForwardActive_ = !player1ForwardActive_;
    }
    validateSingleCharInput(player1ForwardText_);
    
    // Backward text field
    GuiLabel((Rectangle){startX + textFieldWidth + 10, startY + spacing, textFieldWidth, 20}, "Backward");
    if (GuiTextBox((Rectangle){startX + textFieldWidth + 10, startY + spacing * 2, textFieldWidth, 20}, player1BackwardText_, 256, player1BackwardActive_)) {
        player1BackwardActive_ = !player1BackwardActive_;
    }
    validateSingleCharInput(player1BackwardText_);
    
    // Left text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing, textFieldWidth, 20}, "Left");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 2, textFieldWidth, 20}, player1LeftText_, 256, player1LeftActive_)) {
        player1LeftActive_ = !player1LeftActive_;
    }
    validateSingleCharInput(player1LeftText_);
    
    // Right text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing, textFieldWidth, 20}, "Right");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 2, textFieldWidth, 20}, player1RightText_, 256, player1RightActive_)) {
        player1RightActive_ = !player1RightActive_;
    }
    validateSingleCharInput(player1RightText_);
    
    // Save button
    if (GuiButton((Rectangle){startX + (textFieldWidth + 10) * 4, startY + spacing * 2, 100, 20}, "Save Changes")) {
        savePlayer1Settings();
    }
}

void ControlPanel::renderPlayer2Tab() {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    float spacing = 30;
    float textFieldWidth = 100;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, "Player 2 Controls");
    
    // Forward text field
    GuiLabel((Rectangle){startX, startY + spacing, textFieldWidth, 20}, "Forward");
    if (GuiTextBox((Rectangle){startX, startY + spacing * 2, textFieldWidth, 20}, player2ForwardText_, 256, player2ForwardActive_)) {
        player2ForwardActive_ = !player2ForwardActive_;
    }
    validateSingleCharInput(player2ForwardText_);
    
    // Backward text field
    GuiLabel((Rectangle){startX + textFieldWidth + 10, startY + spacing, textFieldWidth, 20}, "Backward");
    if (GuiTextBox((Rectangle){startX + textFieldWidth + 10, startY + spacing * 2, textFieldWidth, 20}, player2BackwardText_, 256, player2BackwardActive_)) {
        player2BackwardActive_ = !player2BackwardActive_;
    }
    validateSingleCharInput(player2BackwardText_);
    
    // Left text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing, textFieldWidth, 20}, "Left");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 2, textFieldWidth, 20}, player2LeftText_, 256, player2LeftActive_)) {
        player2LeftActive_ = !player2LeftActive_;
    }
    validateSingleCharInput(player2LeftText_);
    
    // Right text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing, textFieldWidth, 20}, "Right");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 2, textFieldWidth, 20}, player2RightText_, 256, player2RightActive_)) {
        player2RightActive_ = !player2RightActive_;
    }
    validateSingleCharInput(player2RightText_);
    
    // Save button
    if (GuiButton((Rectangle){startX + (textFieldWidth + 10) * 4, startY + spacing * 2, 100, 20}, "Save Changes")) {
        savePlayer2Settings();
    }
}

void ControlPanel::savePlayer1Settings() {
    if (!settings_) return; // Can't save if no settings available
    
    // Update global settings based on text field values
    if (player1ForwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1ForwardText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer1Up(newKey);
    }
    if (player1BackwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1BackwardText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer1Down(newKey);
    }
    if (player1LeftText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1LeftText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer1Left(newKey);
    }
    if (player1RightText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player1RightText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer1Right(newKey);
    }
}

void ControlPanel::savePlayer2Settings() {
    if (!settings_) return; // Can't save if no settings available
    
    // Update global settings based on text field values
    if (player2ForwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2ForwardText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer2Up(newKey);
    }
    if (player2BackwardText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2BackwardText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer2Down(newKey);
    }
    if (player2LeftText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2LeftText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer2Left(newKey);
    }
    if (player2RightText_[0] != '\0') {
        KeyboardKey newKey = charToKey(player2RightText_[0]);
        if (newKey != KEY_NULL) settings_->setPlayer2Right(newKey);
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
    
    if (GuiButton(tabRect1, "Main")) selectedTab_ = 0;
    if (GuiButton(tabRect2, "Player 1")) selectedTab_ = 1;
    if (GuiButton(tabRect3, "Player 2")) selectedTab_ = 2;
    
    // Render content based on selected tab
    switch (selectedTab_) {
        case 0: renderMainTab(); break;
        case 1: renderPlayer1Tab(); break;
        case 2: renderPlayer2Tab(); break;
    }
}

}} // namespace netcode::visualization
