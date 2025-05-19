#include "netcode/visualization/game_window.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) : running_(false) {
    InitWindow(width, height, title);
    SetTargetFPS(60);

    // Initialize player position at origin
    playerPosition_ = { 0.0f, 0.0f, 0.0f };

    // Initialize camera
    camera_.position = { 10.0f, 10.0f, 10.0f };
    camera_.target = { 0.0f, 0.0f, 0.0f };
    camera_.up = { 0.0f, 1.0f, 0.0f };
    camera_.fovy = 45.0f;
    camera_.projection = CAMERA_PERSPECTIVE;
}

GameWindow::~GameWindow() {
    CloseWindow();
}

void GameWindow::run() {
    running_ = true;
    while (running_ && !WindowShouldClose()) {
        processInput();
        update();
        render();
    }
}

void GameWindow::processInput() {
    if (IsKeyDown(KEY_RIGHT)) playerPosition_.x += moveSpeed_;
    if (IsKeyDown(KEY_LEFT)) playerPosition_.x -= moveSpeed_;
    if (IsKeyDown(KEY_UP)) playerPosition_.z -= moveSpeed_;
    if (IsKeyDown(KEY_DOWN)) playerPosition_.z += moveSpeed_;
}

void GameWindow::update() {
    // Update camera to follow player
    camera_.target = playerPosition_;
}

void GameWindow::render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(camera_);
    
    // Draw grid for reference
    DrawGrid(10, 1.0f);
    
    // Draw player cube
    DrawCube(playerPosition_, 1.0f, 1.0f, 1.0f, RED);
    DrawCubeWires(playerPosition_, 1.0f, 1.0f, 1.0f, MAROON);

    EndMode3D();

    // Draw FPS
    DrawFPS(10, 10);

    EndDrawing();
}

}} // namespace netcode::visualization 