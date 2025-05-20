#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height) : running_(false) {
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

    LOG_INFO("Game window created", "GameWindow");
}

GameWindow::~GameWindow() {
    UnloadRenderTexture(rt1_);
    UnloadRenderTexture(rt2_);
    UnloadRenderTexture(rt3_);
    CloseWindow();
    LOG_INFO("Game window closed", "GameWindow");

}

void GameWindow::processEvents() {
    // Add event processing logic here
}

void GameWindow::update() {
    // Add update logic here
}


// NB! This function is what is actually important to understand
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

    if (!status_text_.empty()) {
        DrawText(status_text_.c_str(), 10, 10, 18, BLACK);
    }

    update_network_messages_display();

    EndDrawing();
}

void GameWindow::run() {
    LOG_INFO("Game loop starting", "GameWindow");
    running_ = true;
    while (!WindowShouldClose() && running_) {
        processEvents();
        update();
        render();
    }

    LOG_INFO("Game loop ended", "GameWindow");

}

void GameWindow::set_status_text(const std::string& text) {
    status_text_ = text;
    LOG_DEBUG("Status-text put to: " + text, "GameWindow");
}

void GameWindow::add_network_message(const std::string& message) {
    network_messages_.push(message);
    LOG_DEBUG("Network message added to: " + message, "GameWindow");

    while (network_messages_.size() > MAX_NETWORK_MESSAGES) {
        network_messages_.pop();
    }
}

void GameWindow::update_network_messages_display() {
    int x = 10;
    int y = GetScreenHeight() - 20;
    int lineHeight = 20;

    std::vector<std::string> messages;
    std::queue<std::string> tempQueue = network_messages_;

    while (!tempQueue.empty()) {
        messages.push_back(tempQueue.front());
        tempQueue.pop();
    }

    for (int i = messages.size() - 1; i >= 0; i--) {
        DrawText(messages[i].c_str(), x, y - (lineHeight * (messages.size() - i)), 16, DARKGRAY);
    }
}




}} // namespace netcode::visualization
