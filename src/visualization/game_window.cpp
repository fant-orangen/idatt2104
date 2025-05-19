#include "netcode/visualization/game_window.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) {
    InitWindow(width, height, title);
    SetTargetFPS(60);
    
    // Create three scenes, each taking up one-third of the window
    int viewportWidth = width / 3;

    // Scene 1: the scene for player 1 
    scene1_ = std::make_unique<GameScene>(
        viewportWidth, height,
        0, 0,
        "View 1"
    );

    // Scene 2: the scene for player 2
    scene2_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth, 0,
        "View 2"
    );
    
    scene3_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth * 2, 0,
        "View 3"
    );
}

GameWindow::~GameWindow() {
    CloseWindow();
}

void GameWindow::render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Handle input for the first scene only
    scene1_->handleInput();
    
    // Render each scene
    scene1_->render();
    scene2_->render();
    scene3_->render();
    
    // Draw borders between viewports (thicker lines)
    int viewportWidth = GetScreenWidth() / 3;
    DrawRectangle(viewportWidth - 2, 0, 4, GetScreenHeight(), BLACK);  // First border
    DrawRectangle(viewportWidth * 2 - 2, 0, 4, GetScreenHeight(), BLACK);  // Second border
    
    EndDrawing();
}

void GameWindow::run() {
    while (!WindowShouldClose()) {
        render();
    }
}

}} // namespace netcode::visualization
