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
KeyboardKey PLAYER2_UP = KEY_UP;
KeyboardKey PLAYER2_DOWN = KEY_DOWN;
KeyboardKey PLAYER2_LEFT = KEY_LEFT;
KeyboardKey PLAYER2_RIGHT = KEY_RIGHT;
KeyboardKey PLAYER2_JUMP = KEY_M;

// Camera Controls
KeyboardKey CAMERA_UP = KEY_E;
KeyboardKey CAMERA_DOWN = KEY_Q;
KeyboardKey CAMERA_LEFT = KEY_LEFT_BRACKET;
KeyboardKey CAMERA_RIGHT = KEY_RIGHT_BRACKET;
KeyboardKey CAMERA_ZOOM_IN = KEY_EQUAL;
KeyboardKey CAMERA_ZOOM_OUT = KEY_MINUS;

// Visualization Settings
bool USE_TEXTURED_GROUND = true;  // Default to grid visualization

// Network Delay Settings (in milliseconds)
int CLIENT_TO_SERVER_DELAY = 500;  // Default 100ms delay before client sends to server
int SERVER_TO_CLIENT_DELAY = 500;  // Default 100ms delay before server broadcasts to clients

}}} // namespace netcode::visualization::settings 