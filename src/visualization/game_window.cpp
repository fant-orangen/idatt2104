#include "netcode/visualization/game_window.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) {
    InitWindow(width * 2, height, title);
    SetTargetFPS(60);
    
    // Create three scenes, each taking up one-third of the window
    int viewportWidth = 2 * width / 3;  // Now each viewport gets the original width

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

    rt1_ = LoadRenderTexture(viewportWidth, height);
    rt2_ = LoadRenderTexture(viewportWidth, height);
    rt3_ = LoadRenderTexture(viewportWidth, height);
}

GameWindow::~GameWindow() {
    UnloadRenderTexture(rt1_);
    UnloadRenderTexture(rt2_);
    UnloadRenderTexture(rt3_);
    CloseWindow();
}

void GameWindow::render() {
    // Handle input for the first scene only
    scene1_->handleInput();
    // Render each scene to its RenderTexture
    BeginTextureMode(rt1_);
    ClearBackground(RAYWHITE);
    scene1_->render();
    EndTextureMode();

    BeginTextureMode(rt2_);
    ClearBackground(RAYWHITE);
    scene2_->render();
    EndTextureMode();

    BeginTextureMode(rt3_);
    ClearBackground(RAYWHITE);
    scene3_->render();
    EndTextureMode();

    // Draw the RenderTextures to the window
    BeginDrawing();
    ClearBackground(RAYWHITE);
    int viewportWidth = rt1_.texture.width;
    int height = rt1_.texture.height;
    DrawTextureRec(rt1_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-height}, (Vector2){0, 0}, WHITE);
    DrawTextureRec(rt2_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-height}, (Vector2){(float)viewportWidth, 0}, WHITE);
    DrawTextureRec(rt3_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-height}, (Vector2){(float)viewportWidth * 2, 0}, WHITE);
    // Draw borders
    DrawRectangle(viewportWidth - 2, 0, 4, height, BLACK);
    DrawRectangle(viewportWidth * 2 - 2, 0, 4, height, BLACK);
    EndDrawing();
}

void GameWindow::run() {
    while (!WindowShouldClose()) {
        render();
    }
}

}} // namespace netcode::visualization
