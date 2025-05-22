#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"
#include "netcode/visualization/settings.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const char* title, int width, int height, NetworkUtility::Mode mode)
    : running_(true), activeSceneForCamera_(nullptr), activeSceneIndex_(0), mouseRightPressed_(false) {

    // Set logger level to DEBUG to ensure all messages are logged
    utils::Logger::get_instance().set_level(utils::LogLevel::DEBUG);

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
    
    // Set player references for server and clients
    if (mode == NetworkUtility::Mode::STANDARD) {
        //add_network_message("Initializing networking in STANDARD mode");
        
        // Connect server scene players with server component
        network_->serverToClientsUpdate(
            scene2_->getRedPlayer(),    // Server's red player
            scene1_->getRedPlayer(),    // Client 1's red player
            scene3_->getRedPlayer()     // Client 2's red player
        );
        
        network_->serverToClientsUpdate(
            scene2_->getBluePlayer(),   // Server's blue player
            scene1_->getBluePlayer(),   // Client 1's blue player
            scene3_->getBluePlayer()    // Client 2's blue player
        );
    }
    
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
    // Update network delay settings from control panel
    settings::CLIENT_TO_SERVER_DELAY = static_cast<int>(controlPanel_->getClientToServerDelay());
    settings::SERVER_TO_CLIENT_DELAY = static_cast<int>(controlPanel_->getServerToClientDelay());
    
    // Update network components
    if (network_) {
        // Process any queued network updates
        network_->update();
        
        // Make sure clients interpolate remote entities
        // This is critical for ensuring all clients see other players move smoothly
        auto client1 = network_->getClient1();
        auto client2 = network_->getClient2();
        
        // Update client entities using interpolation system
        if (client1) {
            client1->updateEntities(GetFrameTime());
        }
        
        if (client2) {
            client2->updateEntities(GetFrameTime());
        }
    }
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
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw each scene to its render texture
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

    // Draw render textures to screen
    DrawTextureRec(rt1_.texture,
                   Rectangle{0, 0, static_cast<float>(rt1_.texture.width), static_cast<float>(-rt1_.texture.height)},
                   Vector2{0, 0}, WHITE);

    DrawTextureRec(rt2_.texture,
                   Rectangle{0, 0, static_cast<float>(rt2_.texture.width), static_cast<float>(-rt2_.texture.height)},
                   Vector2{static_cast<float>(rt1_.texture.width), 0}, WHITE);

    DrawTextureRec(rt3_.texture,
                   Rectangle{0, 0, static_cast<float>(rt3_.texture.width), static_cast<float>(-rt3_.texture.height)},
                   Vector2{static_cast<float>(rt1_.texture.width + rt2_.texture.width), 0}, WHITE);

    // Draw control panel
    controlPanel_->render();

    // Draw status text if any
    if (!status_text_.empty()) {
        DrawText(status_text_.c_str(), 10, GetScreenHeight() - 30, 20, DARKGRAY);
    }

    // Draw network messages
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
        scene1_->handleInput();  // Process WASD controls for red player
        scene3_->handleInput();  // Process arrow key controls for blue player
    } else {
        // Handle control panel input if mouse is in panel area
        controlPanel_->handleMouseInteraction(mousePos);
    }

    // Process network updates
    if (network_) {
        // Only send updates if we have valid player references
        auto redPlayer1 = scene1_->getRedPlayer();
        auto redPlayerServer = scene2_->getRedPlayer();
        auto bluePlayer2 = scene3_->getBluePlayer();
        auto bluePlayerServer = scene2_->getBluePlayer();

        // Send client 1 (red player) updates to server only if there's movement or jump
        if (redPlayer1 && redPlayerServer) {
            Vector3 redMovement = scene1_->getRedMovementDirection();
            bool redJump = scene1_->getRedJumpRequested();
            Vector3 redPosition = toVector3(redPlayer1->getPosition());  // Use conversion utility
            
            // Send update if there's movement, jump, or player is above ground level (1.0f)
            if (redMovement.x != 0 || redMovement.z != 0 || redJump || redPosition.y > 1.0f) {
                network_->clientToServerUpdate(
                    redPlayer1,
                    redPlayerServer,
                    redMovement,
                    redJump
                );
                
                // Display network debug info
                std::string msg = "Client 1 sending movement: [" + 
                                std::to_string(redMovement.x) + "," +
                                std::to_string(redMovement.z) + "]";
                if (redJump) msg += " + JUMP";
                if (redPosition.y > 1.0f) msg += " (airborne)";
                add_network_message(msg);
            }
        }

        // Send client 2 (blue player) updates to server only if there's movement or jump
        if (bluePlayer2 && bluePlayerServer) {
            Vector3 blueMovement = scene3_->getBlueMovementDirection();
            bool blueJump = scene3_->getBlueJumpRequested();
            Vector3 bluePosition = toVector3(bluePlayer2->getPosition());  // Use conversion utility
            
            // Send update if there's movement, jump, or player is above ground level (1.0f)
            if (blueMovement.x != 0 || blueMovement.z != 0 || blueJump || bluePosition.y > 1.0f) {
                network_->clientToServerUpdate(
                    bluePlayer2,
                    bluePlayerServer,
                    blueMovement,
                    blueJump
                );
                
                // Display network debug info
                std::string msg = "Client 2 sending movement: [" + 
                                std::to_string(blueMovement.x) + "," +
                                std::to_string(blueMovement.z) + "]";
                if (blueJump) msg += " + JUMP";
                if (bluePosition.y > 1.0f) msg += " (airborne)";
                add_network_message(msg);
            }
        }

        // These server-to-client updates are now handled by the Server/Client classes
        // We only need to perform them in TEST mode
        if (network_->isTestMode()) {
            // Update client views with server state in TEST mode only
            if (redPlayerServer) {
                network_->serverToClientsUpdate(
                    redPlayerServer,
                    redPlayer1,  // Client 1's view of red player
                    scene3_->getRedPlayer()  // Client 2's view of red player
                );
            }
            
            if (bluePlayerServer) {
                network_->serverToClientsUpdate(
                    bluePlayerServer,
                    scene1_->getBluePlayer(),  // Client 1's view of blue player
                    bluePlayer2  // Client 2's view of blue player
                );
            }
        }

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
    // Skip adding network-related messages to GUI display
    if (message.find("Client") != std::string::npos || 
        message.find("Server") != std::string::npos || 
        message.find("NetworkUtility") != std::string::npos) {
        // Still log the message but don't add to GUI queue
        LOG_DEBUG(message, "GameWindow");
        return;
    }

    // Only non-network messages get added to the GUI display queue
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
