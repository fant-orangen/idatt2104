#include "netcode/visualization/game_scene.hpp"

namespace netcode {
namespace visualization {

GameScene::GameScene(int viewportWidth, int viewportHeight, ViewType type,
                     float x, float y, const char* label)
    : viewType_(type), label_(label) {
    
    bounds_ = {x, y, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    entityController_ = std::make_unique<LocalEntityController>();
    setupCamera();
}

GameScene::~GameScene() = default;

void GameScene::setupCamera() {
    // Each scene has its own independent camera and player
    camera_ = {
        {0.0f, 10.0f, 20.0f},    // Camera position behind and above
        {0.0f, 0.0f, 0.0f},      // Looking at center
        {0, 1.0f, 0},            // Up vector
        45.0f,                    // FOV
        CAMERA_PERSPECTIVE
    };
    
    // Initialize this scene's player at center
    entityController_->getPlayer().setPosition({0.0f, 0.0f, 0.0f});
}

void GameScene::handleInput() {
    // Each scene can control its own player
    Vector3 movement = {0, 0, 0};
    if (IsKeyDown(KEY_RIGHT)) movement.x = 1;    // Right moves right (X axis)
    if (IsKeyDown(KEY_LEFT)) movement.x = -1;    // Left moves left (X axis)
    if (IsKeyDown(KEY_UP)) movement.z = -1;      // Up moves forward (Z axis)
    if (IsKeyDown(KEY_DOWN)) movement.z = 1;     // Down moves backward (Z axis)

    if (movement.x != 0 || movement.z != 0) {
        entityController_->updatePlayerOne(movement);
    }
}

void GameScene::update() {
    entityController_->update();
    Vector3 playerPos = entityController_->getPlayer().getPosition();
    
    // Keep camera at fixed offset from this scene's player
    camera_.position = {
        playerPos.x,           // Follow player X
        playerPos.y + 10.0f,   // Fixed height above
        playerPos.z + 20.0f    // Fixed distance behind
    };
    camera_.target = playerPos;  // Always look at player
}

void GameScene::updatePlayerState(const Vector3& position) {
    entityController_->getPlayer().setPosition(position);
}

void GameScene::drawGrid() const {
    // Draw a larger grid for better orientation
    DrawGrid(20, 1.0f);
    
    // Draw coordinate axes for reference
    DrawLine3D({0,0,0}, {5,0,0}, RED);    // X axis
    DrawLine3D({0,0,0}, {0,5,0}, GREEN);  // Y axis
    DrawLine3D({0,0,0}, {0,0,5}, BLUE);   // Z axis
}

void GameScene::drawScene() {
    BeginMode3D(camera_);
    
    // Draw the environment
    drawGrid();
    
    // Draw this scene's player
    entityController_->getPlayer().draw();
    
    EndMode3D();
}

void GameScene::render() {
    // Set up the viewport for this scene
    BeginScissorMode(bounds_.x, bounds_.y, bounds_.width, bounds_.height);
    ClearBackground(RAYWHITE);
    
    // Draw viewport label with background
    DrawRectangle(bounds_.x, bounds_.y, bounds_.width, 30, Fade(BLACK, 0.5f));
    DrawText(label_, bounds_.x + 10, bounds_.y + 5, 20, WHITE);
    
    // Draw the 3D scene
    BeginMode3D(camera_);
    drawGrid();
    entityController_->getPlayer().draw();
    EndMode3D();
    
    EndScissorMode();
}

}} // namespace netcode::visualization 