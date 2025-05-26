#pragma once

#include "raylib.h"

namespace netcode {
namespace visualization {
namespace settings {

// Player 1 (Red) Controls
extern KeyboardKey PLAYER1_UP;
extern KeyboardKey PLAYER1_DOWN;
extern KeyboardKey PLAYER1_LEFT;
extern KeyboardKey PLAYER1_RIGHT;
extern KeyboardKey PLAYER1_JUMP;

// Player 2 (Blue) Controls
extern KeyboardKey PLAYER2_UP;
extern KeyboardKey PLAYER2_DOWN;
extern KeyboardKey PLAYER2_LEFT;
extern KeyboardKey PLAYER2_RIGHT;
extern KeyboardKey PLAYER2_JUMP;

// Camera Controls
extern KeyboardKey CAMERA_UP;
extern KeyboardKey CAMERA_DOWN;
extern KeyboardKey CAMERA_LEFT;
extern KeyboardKey CAMERA_RIGHT;
extern KeyboardKey CAMERA_ZOOM_IN;
extern KeyboardKey CAMERA_ZOOM_OUT;

// Visualization Settings
extern bool USE_TEXTURED_GROUND;  // Default to grid visualization

// Network Delay Settings (in milliseconds)
extern int CLIENT_TO_SERVER_DELAY;  // Delay before client sends to server
extern int SERVER_TO_CLIENT_DELAY;  // Delay before server broadcasts to clients

// Reconciliation Settings
extern bool ENABLE_PREDICTION;  // Enable reconciliation for player controls
extern bool ENABLE_INTERPOLATION;  // Enable interpolation for player controls

}}} // namespace netcode::visualization::settings
