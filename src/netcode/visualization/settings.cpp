#include "netcode/visualization/settings.hpp"

namespace netcode {
namespace visualization {
namespace settings {

// Player 1 (Red) Controls
KeyboardKey PLAYER1_UP = KEY_W;
KeyboardKey PLAYER1_DOWN = KEY_S;
KeyboardKey PLAYER1_LEFT = KEY_A;
KeyboardKey PLAYER1_RIGHT = KEY_D;
KeyboardKey PLAYER1_JUMP = KEY_SPACE;

// Player 2 (Blue) Controls
KeyboardKey PLAYER2_UP = KEY_I;
KeyboardKey PLAYER2_DOWN = KEY_K;
KeyboardKey PLAYER2_LEFT = KEY_J;
KeyboardKey PLAYER2_RIGHT = KEY_L;
KeyboardKey PLAYER2_JUMP = KEY_M;

// Camera Controls
KeyboardKey CAMERA_UP = KEY_T;
KeyboardKey CAMERA_DOWN = KEY_G;
KeyboardKey CAMERA_LEFT = KEY_H;
KeyboardKey CAMERA_RIGHT = KEY_F;
KeyboardKey CAMERA_ZOOM_IN = KEY_EQUAL;
KeyboardKey CAMERA_ZOOM_OUT = KEY_MINUS;

// Visualization Settings
bool USE_TEXTURED_GROUND = true;  // Default to grid visualization

// Network Delay Settings (in milliseconds)
int CLIENT_TO_SERVER_DELAY = 10; // Delay before client sends original movement request to server
int SERVER_TO_CLIENT_DELAY = 50; // Delay before server broadcasts position update to clients

// Reconciliation Settings
bool ENABLE_PREDICTION = false;  // Default to disabled
bool ENABLE_INTERPOLATION = false;  // Default to disabled

}}} // namespace netcode::visualization::settings 