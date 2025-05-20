#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height, NetworkUtility::Mode mode)
    : running_(true), activeSceneForCamera_(nullptr), activeSceneIndex_(0), mouseRightPressed_(false) {

    // Initialize window with extra height for control panel
    InitWindow(width * 2, height + CONTROL_PANEL_HEIGHT, title);
    SetTargetFPS(60);
    
    // Create scenes with original height
    int viewportWidth = 2 * width / 3;

    // Scene 1: Player 1 controls the red player with WASD
    scene1_ = std::make_unique<GameScene>(
        viewportWidth, height,
        0, 0,
        "Player 1 (F1)"
    );

    // Scene 2: Server view (no controls)
    scene2_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth, 0,
        "Server (F2)"
    );
    
    // Scene 3: Player 2 controls the blue player with arrow keys
    scene3_ = std::make_unique<GameScene>(
        viewportWidth, height,
        viewportWidth * 2, 0,
        "Player 2 (F3)"
    );

    rt1_ = LoadRenderTexture(viewportWidth, height);
    rt2_ = LoadRenderTexture(viewportWidth, height);
    rt3_ = LoadRenderTexture(viewportWidth, height);

    // Create control panel at the bottom of the window
    controlPanel_ = std::make_unique<ControlPanel>(0, height, width * 2, CONTROL_PANEL_HEIGHT);

    // Create network utility with specified mode
    network_ = std::make_unique<NetworkUtility>(mode);

    // Set default scene for camera control
    activeSceneForCamera_ = scene1_.get();
    activeSceneIndex_ = 1;
    prevMousePos_ = GetMousePosition();

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

void GameWindow::handleCameraInput() {
    // Switch active camera control scene with F1, F2, F3 keys
    if (IsKeyPressed(KEY_F1)) {
        activeSceneForCamera_ = scene1_.get();
        activeSceneIndex_ = 1;
    }
    else if (IsKeyPressed(KEY_F2)) {
        activeSceneForCamera_ = scene2_.get();
        activeSceneIndex_ = 2;
    }
    else if (IsKeyPressed(KEY_F3)) {
        activeSceneForCamera_ = scene3_.get();
        activeSceneIndex_ = 3;
    }

    if (!activeSceneForCamera_) return;

    // Pan camera with right mouse button drag
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 currentMousePos = GetMousePosition();

        if (!mouseRightPressed_) {
            // Just pressed - store initial position
            prevMousePos_ = currentMousePos;
            mouseRightPressed_ = true;
        } else {
            // Calculate mouse delta for panning
            float deltaX = (currentMousePos.x - prevMousePos_.x) * CAMERA_PAN_SPEED;
            float deltaY = (currentMousePos.y - prevMousePos_.y) * CAMERA_PAN_SPEED;

            activeSceneForCamera_->panCamera(deltaX, deltaY);
            prevMousePos_ = currentMousePos;
        }
    } else {
        mouseRightPressed_ = false;
    }

    // Move camera up/down with U/J
    if (IsKeyDown(KEY_U)) {
        activeSceneForCamera_->moveCameraUp(CAMERA_MOVE_SPEED);
    }
    else if (IsKeyDown(KEY_J)) {
        activeSceneForCamera_->moveCameraUp(-CAMERA_MOVE_SPEED);
    }

    // Move camera left/right with H/K
    if (IsKeyDown(KEY_K)) {
        activeSceneForCamera_->moveCameraRight(-CAMERA_MOVE_SPEED);
    }
    else if (IsKeyDown(KEY_H)) {
        activeSceneForCamera_->moveCameraRight(CAMERA_MOVE_SPEED);
    }

    // Zoom with mouse wheel
    float mouseWheelMove = GetMouseWheelMove();
    if (mouseWheelMove != 0) {
        activeSceneForCamera_->zoomCamera(-mouseWheelMove * CAMERA_ZOOM_SPEED);
    }
}

void GameWindow::render() {
    // Only handle camera controls if no text field is active
    if (!controlPanel_->isTextFieldActive()) {
        handleCameraInput();

        // Handle input from both player scenes
        scene1_->handleInput();  // Process WASD controls for red player
        scene3_->handleInput();  // Process arrow key controls for blue player
    }
    
    // Update server state based on client inputs
    if (network_) {
        // Update server's red player based on Player 1's input
        network_->clientToServerUpdate(
            scene1_->getRedPlayer(),
            scene2_->getRedPlayer(),
            scene1_->getRedMovementDirection(),
            scene1_->getRedJumpRequested()
        );
        
        // Update server's blue player based on Player 2's input
        network_->clientToServerUpdate(
            scene3_->getBluePlayer(),
            scene2_->getBluePlayer(),
            scene3_->getBlueMovementDirection(),
            scene3_->getBlueJumpRequested()
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
    int gameHeight = GetScreenHeight() - CONTROL_PANEL_HEIGHT;

    DrawTextureRec(rt1_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-gameHeight}, (Vector2){0, 0}, WHITE);
    DrawTextureRec(rt2_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-gameHeight}, (Vector2){(float)viewportWidth, 0}, WHITE);
    DrawTextureRec(rt3_.texture, (Rectangle){0, 0, (float)viewportWidth, (float)-gameHeight}, (Vector2){(float)viewportWidth * 2, 0}, WHITE);

    // Draw borders
    DrawRectangle(viewportWidth - 2, 0, 4, gameHeight, BLACK);
    DrawRectangle(viewportWidth * 2 - 2, 0, 4, gameHeight, BLACK);

    // Draw horizontal line separating game views from control panel
    DrawRectangle(0, gameHeight - 2, GetScreenWidth(), 4, BLACK);

    // Render control panel
    controlPanel_->render();

    // Display active camera indicator
    if (activeSceneIndex_ > 0) {
        DrawText(TextFormat("Camera Control: View %d", activeSceneIndex_), 10, gameHeight - 30, 20, DARKGRAY);
    }

    if (!status_text_.empty()) {
        DrawText(status_text_.c_str(), 10, 10, 18, BLACK);
    }

    update_network_messages_display();


    EndDrawing();
}

void GameWindow::handleInput() {
    // Check if any text field is active in the control panel
    bool textFieldActive = controlPanel_->isTextFieldActive();

    // Handle camera input only if mouse is in game area AND no text field is active
    Vector2 mousePos = GetMousePosition();
    if (mousePos.y < (GetScreenHeight() - CONTROL_PANEL_HEIGHT) && !textFieldActive) {
        handleCameraInput();

        // Handle game scene input
        scene1_->handleInput();
        scene3_->handleInput();
    } else {
        // Handle control panel input if mouse is in panel area
        controlPanel_->handleMouseInteraction(mousePos);
    }

    // Process network updates
    if (network_) {
        // Update server's red player based on Player 1's input
        network_->clientToServerUpdate(
            scene1_->getRedPlayer(),
            scene2_->getRedPlayer(),
            scene1_->getRedMovementDirection(),
            scene1_->getRedJumpRequested()
        );

        // Update server's blue player based on Player 2's input
        network_->clientToServerUpdate(
            scene3_->getBluePlayer(),
            scene2_->getBluePlayer(),
            scene3_->getBlueMovementDirection(),
            scene3_->getBlueJumpRequested()
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

        network_->update();
    }
}

void GameWindow::run() {
    LOG_INFO("Game loop starting", "GameWindow");
    running_ = true;
    while (!WindowShouldClose() && running_) {
        processEvents();
        update();
        handleInput();
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
