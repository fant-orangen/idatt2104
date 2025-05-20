#pragma once

#include "raylib.h"

namespace netcode {
namespace visualization {
namespace settings {

// Player 1 (Red) Controls
constexpr KeyboardKey PLAYER1_UP = KEY_W;
constexpr KeyboardKey PLAYER1_DOWN = KEY_S;
constexpr KeyboardKey PLAYER1_LEFT = KEY_A;
constexpr KeyboardKey PLAYER1_RIGHT = KEY_D;
constexpr KeyboardKey PLAYER1_JUMP = KEY_SPACE;

// Player 2 (Blue) Controls
constexpr KeyboardKey PLAYER2_UP = KEY_UP;
constexpr KeyboardKey PLAYER2_DOWN = KEY_DOWN;
constexpr KeyboardKey PLAYER2_LEFT = KEY_LEFT;
constexpr KeyboardKey PLAYER2_RIGHT = KEY_RIGHT;
constexpr KeyboardKey PLAYER2_JUMP = KEY_M;

// Camera Controls
constexpr KeyboardKey CAMERA_UP = KEY_E;
constexpr KeyboardKey CAMERA_DOWN = KEY_Q;
constexpr KeyboardKey CAMERA_LEFT = KEY_LEFT_BRACKET;
constexpr KeyboardKey CAMERA_RIGHT = KEY_RIGHT_BRACKET;
constexpr KeyboardKey CAMERA_ZOOM_IN = KEY_EQUAL;
constexpr KeyboardKey CAMERA_ZOOM_OUT = KEY_MINUS;

// Visualization Settings
constexpr bool USE_TEXTURED_GROUND = false;  // Default to grid visualization

}}} // namespace netcode::visualization::settings
