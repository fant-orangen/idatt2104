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
        case KEY_ZERO: return "0";
        case KEY_ONE: return "1";
        case KEY_TWO: return "2";
        case KEY_THREE: return "3";
        case KEY_FOUR: return "4";
        case KEY_FIVE: return "5";
        case KEY_SIX: return "6";
        case KEY_SEVEN: return "7";
        case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";
        default: 
            display[0] = (key >= 32 && key <= 126) ? key : '?';
            return display;
    }
}

// Helper function to convert char to key code
KeyboardKey charToKey(char c) {
    // Handle letters
    if (isalpha(c)) {
        c = toupper(c);
        if (c >= 'A' && c <= 'Z') return static_cast<KeyboardKey>(c);
    }
    
    // Handle numbers
    if (isdigit(c)) {
        switch(c) {
            case '0': return KEY_ZERO;
            case '1': return KEY_ONE;
            case '2': return KEY_TWO;
            case '3': return KEY_THREE;
            case '4': return KEY_FOUR;
            case '5': return KEY_FIVE;
            case '6': return KEY_SIX;
            case '7': return KEY_SEVEN;
            case '8': return KEY_EIGHT;
            case '9': return KEY_NINE;
        }
    }
    
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
    float startY = bounds_.y + 50;
    float spacing = 45;
    float sectionWidth = 200.0f;
    
    // Left Section - Network Delays
    GuiLabel((Rectangle){startX, startY, sectionWidth + 200, 40},
             "Network Delays (ms):\nSimulate real-world latency between client and server communication.\nHigher values = more lag.");
    
    // Network delay controls
    float controlsY = startY + 20;  // Space after info text
    
    // Client to Server delay slider with label
    GuiLabel((Rectangle){startX, controlsY + spacing - 15, sectionWidth, 20.0f}, "Client -> Server Delay");
    GuiSlider((Rectangle){startX, controlsY + spacing, sectionWidth, 20.0f}, 
              "", TextFormat("%.0f", clientToServerDelay_), 
              &clientToServerDelay_, 0, 500);
    
    // Server to Client delay slider with label
    GuiLabel((Rectangle){startX, controlsY + spacing * 2 - 15, sectionWidth, 20.0f}, "Server -> Client Delay");
    GuiSlider((Rectangle){startX, controlsY + spacing * 2, sectionWidth, 20.0f}, 
              "", TextFormat("%.0f", serverToClientDelay_), 
              &serverToClientDelay_, 0, 500);

    // Right Section - Game Settings
    float rightStartX = startX + sectionWidth + 300;  // Increased gap between sections to 300 pixels

    // Information text
    GuiLabel((Rectangle){rightStartX, startY, sectionWidth + 200, 40},
             "Game Settings:\nPrediction: Reduces perceived lag\n"
             "Interpolation: Smooths movement of other players");

    // Game Settings controls
    if (settings_) {
        bool prediction = settings_->isPredictionEnabled();
        bool interpolation = settings_->isInterpolationEnabled();
        
        if (GuiCheckBox((Rectangle){rightStartX, controlsY + spacing, 20, 20}, "Enable Prediction", &prediction)) {
            settings_->setPredictionEnabled(prediction);
        }
        if (GuiCheckBox((Rectangle){rightStartX, controlsY + spacing * 2, 20, 20}, "Enable Interpolation", &interpolation)) {
            settings_->setInterpolationEnabled(interpolation);
        }
    }

    // Important information
    float reminderX = rightStartX + sectionWidth + 200;  // Position to the right of Game Settings
    GuiLabel((Rectangle){reminderX, startY, sectionWidth + 300, 40},
             "IMPORTANT NOTE:\nAfter changing settings, you MUST click on one of \nthe game windows for the changes to take effect!");

    // Control information
    float controlInfoY = startY + 20;  // Position below the important note
    GuiLabel((Rectangle){reminderX, controlInfoY, sectionWidth + 300, 150},
             "GAME CONTROLS:\n"
             "Jump: SPACE or M\n"
             "Switch Windows: F1, F2, F3\n"
             "Change View Perspective: T, F, G, H\n"
             "Movement Controls: Check Player 1 & 2 Panels");
}

void ControlPanel::renderPlayerTab(int playerNum, const PlayerControls& controls) {
    float startX = bounds_.x + 10;
    float startY = bounds_.y + 50;
    float spacing = 30;
    float textFieldWidth = 100;
    
    GuiLabel((Rectangle){startX, startY, 200, 20}, TextFormat("Player %d Controls", playerNum));
    
    // Add informative text
    GuiLabel((Rectangle){startX, startY + spacing, bounds_.width - 20, 40}, 
             TextFormat("Configure the keyboard controls for Player %d.\nEnter a single letter or number for each control.", playerNum));
    
    // Forward text field
    GuiLabel((Rectangle){startX, startY + spacing * 3, textFieldWidth, 20}, "Forward");
    if (GuiTextBox((Rectangle){startX, startY + spacing * 4, textFieldWidth, 20}, controls.forwardText, 256, *controls.forwardActive)) {
        *controls.forwardActive = !*controls.forwardActive;
    }
    validateSingleCharInput(controls.forwardText);
    
    // Backward text field
    GuiLabel((Rectangle){startX + textFieldWidth + 10, startY + spacing * 3, textFieldWidth, 20}, "Backward");
    if (GuiTextBox((Rectangle){startX + textFieldWidth + 10, startY + spacing * 4, textFieldWidth, 20}, controls.backwardText, 256, *controls.backwardActive)) {
        *controls.backwardActive = !*controls.backwardActive;
    }
    validateSingleCharInput(controls.backwardText);
    
    // Left text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 3, textFieldWidth, 20}, "Left");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 2, startY + spacing * 4, textFieldWidth, 20}, controls.leftText, 256, *controls.leftActive)) {
        *controls.leftActive = !*controls.leftActive;
    }
    validateSingleCharInput(controls.leftText);
    
    // Right text field
    GuiLabel((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 3, textFieldWidth, 20}, "Right");
    if (GuiTextBox((Rectangle){startX + (textFieldWidth + 10) * 3, startY + spacing * 4, textFieldWidth, 20}, controls.rightText, 256, *controls.rightActive)) {
        *controls.rightActive = !*controls.rightActive;
    }
    validateSingleCharInput(controls.rightText);
    
    // Save button
    if (GuiButton((Rectangle){startX + (textFieldWidth + 10) * 4, startY + spacing * 4, 100, 20}, "Save Changes")) {
        savePlayerSettings(playerNum);
    }
}

void ControlPanel::savePlayerSettings(int playerNum) {
    if (!settings_) return; // Can't save if no settings available
    
    char* forwardText = (playerNum == 1) ? player1ForwardText_ : player2ForwardText_;
    char* backwardText = (playerNum == 1) ? player1BackwardText_ : player2BackwardText_;
    char* leftText = (playerNum == 1) ? player1LeftText_ : player2LeftText_;
    char* rightText = (playerNum == 1) ? player1RightText_ : player2RightText_;
    
    // Update global settings based on text field values
    if (forwardText[0] != '\0') {
        KeyboardKey newKey = charToKey(forwardText[0]);
        if (newKey != KEY_NULL) {
            if (playerNum == 1) settings_->setPlayer1Up(newKey);
            else settings_->setPlayer2Up(newKey);
        }
    }
    if (backwardText[0] != '\0') {
        KeyboardKey newKey = charToKey(backwardText[0]);
        if (newKey != KEY_NULL) {
            if (playerNum == 1) settings_->setPlayer1Down(newKey);
            else settings_->setPlayer2Down(newKey);
        }
    }
    if (leftText[0] != '\0') {
        KeyboardKey newKey = charToKey(leftText[0]);
        if (newKey != KEY_NULL) {
            if (playerNum == 1) settings_->setPlayer1Left(newKey);
            else settings_->setPlayer2Left(newKey);
        }
    }
    if (rightText[0] != '\0') {
        KeyboardKey newKey = charToKey(rightText[0]);
        if (newKey != KEY_NULL) {
            if (playerNum == 1) settings_->setPlayer1Right(newKey);
            else settings_->setPlayer2Right(newKey);
        }
    }
}

void ControlPanel::renderPlayer1Tab() {
    PlayerControls controls = {
        player1ForwardText_, player1BackwardText_, player1LeftText_, player1RightText_,
        &player1ForwardActive_, &player1BackwardActive_, &player1LeftActive_, &player1RightActive_
    };
    renderPlayerTab(1, controls);
}

void ControlPanel::renderPlayer2Tab() {
    PlayerControls controls = {
        player2ForwardText_, player2BackwardText_, player2LeftText_, player2RightText_,
        &player2ForwardActive_, &player2BackwardActive_, &player2LeftActive_, &player2RightActive_
    };
    renderPlayerTab(2, controls);
}

void ControlPanel::render() {
    // Draw panel background
    DrawRectangle(bounds_.x, bounds_.y, bounds_.width, bounds_.height, LIGHTGRAY);
    DrawRectangleLines(bounds_.x, bounds_.y, bounds_.width, bounds_.height, DARKGRAY);
    
    // Draw tabs
    Rectangle tabRect1 = {bounds_.x + 10, bounds_.y + 5, 100, 30};
    Rectangle tabRect2 = {bounds_.x + 120, bounds_.y + 5, 100, 30};
    Rectangle tabRect3 = {bounds_.x + 230, bounds_.y + 5, 100, 30};

    // Store original button style
    int originalBase = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
    int hoverColor = GuiGetStyle(BUTTON, BASE_COLOR_FOCUSED);

    // Set color for selected tab
    if (selectedTab_ == 0) GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, hoverColor);
    if (GuiButton(tabRect1, "Main")) selectedTab_ = 0;
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, originalBase);

    if (selectedTab_ == 1) GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, hoverColor);
    if (GuiButton(tabRect2, "Player 1")) selectedTab_ = 1;
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, originalBase);

    if (selectedTab_ == 2) GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, hoverColor);
    if (GuiButton(tabRect3, "Player 2")) selectedTab_ = 2;
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, originalBase);

    // Render content based on selected tab
    switch (selectedTab_) {
        case 0: renderMainTab(); break;
        case 1: renderPlayer1Tab(); break;
        case 2: renderPlayer2Tab(); break;
    }
}

}} // namespace netcode::visualization
