#include "netcode/visualization/game_window.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) : running_(false) {
    InitWindow(width, height, title);
    SetTargetFPS(60);
    createScenes(width, height);
}

GameWindow::~GameWindow() {
    CloseWindow();
}

void GameWindow::createScenes(int width, int height) {
    int viewportWidth = width / 3;
    
    // Create three scenes, each taking up one-third of the window
    playerOneScene_ = std::make_unique<GameScene>(
        viewportWidth, height, 
        ViewType::PLAYER_ONE,
        0, 0,
        "Player One View"
    );
    
    serverScene_ = std::make_unique<GameScene>(
        viewportWidth, height,
        ViewType::SERVER,
        viewportWidth, 0,
        "Server View"
    );
    
    playerTwoScene_ = std::make_unique<GameScene>(
        viewportWidth, height,
        ViewType::PLAYER_TWO,
        viewportWidth * 2, 0,
        "Player Two View"
    );
}

void GameWindow::processEvents() {
    running_ = !WindowShouldClose();
}

void GameWindow::update() {
    // Each scene handles its own input and updates independently
    playerOneScene_->handleInput();
    playerOneScene_->update();
    
    serverScene_->handleInput();
    serverScene_->update();
    
    playerTwoScene_->handleInput();
    playerTwoScene_->update();
}

void GameWindow::render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Draw borders between viewports
    int viewportWidth = GetScreenWidth() / 3;
    DrawLine(viewportWidth, 0, viewportWidth, GetScreenHeight(), BLACK);
    DrawLine(viewportWidth * 2, 0, viewportWidth * 2, GetScreenHeight(), BLACK);
    
    playerOneScene_->render();
    serverScene_->render();
    playerTwoScene_->render();
    
    // Draw FPS counter
    DrawFPS(10, 10);
    
    EndDrawing();
}

void GameWindow::run() {
    running_ = true;
    while (running_) {
        processEvents();
        update();
        render();
    }
}

}} // namespace netcode::visualization 