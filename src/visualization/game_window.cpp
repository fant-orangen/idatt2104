#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) {
    InitWindow(width * 2, height, title);
    SetTargetFPS(60);
    
    // Create network utility in test mode
    network_ = std::make_unique<NetworkUtility>(NetworkUtility::Mode::TEST);
    
    // Create three scenes, each taking up one-third of the window
    int viewportWidth = 2 * width / 3;  // Each viewport gets the original width

    // Scene 1: Player 1 controls the red player with WASD
    scene1_ = std::make_unique<GameScene>(
        viewportWidth, height,
        0, 0,
        "Player 1",
        KEY_W, KEY_S, KEY_A, KEY_D,  // Red player uses WASD
        KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL  // No blue player controls here
    );

    // Scene 2: Server view (no controls)
    scene2_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth, 0,
        "Server",
        KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,  // No controls for red player
        KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL   // No controls for blue player
    );
    
    // Scene 3: Player 2 controls the blue player with arrow keys
    scene3_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth * 2, 0,
        "Player 2",
        KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,  // No red player controls here
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT    // Blue player uses arrow keys
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
    // Handle input from both player scenes
    scene1_->handleInput();  // Process WASD controls for red player
    scene3_->handleInput();  // Process arrow key controls for blue player
    
    // Update server state based on client inputs
    if (network_) {
        // Update server's red player based on Player 1's input
        network_->clientToServerUpdate(
            scene1_->getRedPlayer(),
            scene2_->getRedPlayer(),
            scene1_->getRedMovementDirection()
        );
        
        // Update server's blue player based on Player 2's input
        network_->clientToServerUpdate(
            scene3_->getBluePlayer(),
            scene2_->getBluePlayer(),
            scene3_->getBlueMovementDirection()
        );
        
        // Propagate server state back to clients
        network_->serverToClientsUpdate(
            scene2_->getRedPlayer(),
            scene1_->getRedPlayer(),
            scene3_->getRedPlayer()
        );
        network_->serverToClientsUpdate(
            scene2_->getBluePlayer(),
            scene1_->getBluePlayer(),
            scene3_->getBluePlayer()
        );

        // Process any pending updates
        network_->update();
    }
    
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
