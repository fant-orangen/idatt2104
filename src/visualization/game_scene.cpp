#include "netcode/visualization/game_scene.hpp"
#include "rlgl.h"
#include "raymath.h" // For vector operations and DEG2RAD

namespace netcode {
namespace visualization {

FramebufferRect toFramebufferRect(const Rectangle& logicalRect) {
    float scaleX = (float)GetRenderWidth() / GetScreenWidth();
    float scaleY = (float)GetRenderHeight() / GetScreenHeight();
    return {
        (int)(logicalRect.x * scaleX),
        (int)(logicalRect.y * scaleY),
        (int)(logicalRect.width * scaleX),
        (int)(logicalRect.height * scaleY)
    };
}

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label,
                     KeyboardKey redUp, KeyboardKey redDown, KeyboardKey redLeft, KeyboardKey redRight,
                     KeyboardKey blueUp, KeyboardKey blueDown, KeyboardKey blueLeft, KeyboardKey blueRight)
    : label_(label),
      redUp_(redUp), redDown_(redDown), redLeft_(redLeft), redRight_(redRight),
      blueUp_(blueUp), blueDown_(blueDown), blueLeft_(blueLeft), blueRight_(blueRight) {
    bounds_ = {x, y, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    float aspect = bounds_.width / bounds_.height;
    camera_ = {
        {0.0f, 15.0f, 25.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        60.0f,
        CAMERA_PERSPECTIVE
    };
    
    // Create player instances with their respective models
    redPlayer_ = std::make_shared<Player>(PlayerType::RED_PLAYER, Vector3{-2.0f, 1.0f, 0.0f}, RED);
    bluePlayer_ = std::make_shared<Player>(PlayerType::BLUE_PLAYER, Vector3{2.0f, 1.0f, 0.0f}, BLUE);
}

void GameScene::handleInput() {
    // Reset movement directions and jump requests
    redMoveDir_ = {0.0f, 0.0f, 0.0f};
    blueMoveDir_ = {0.0f, 0.0f, 0.0f};
    redJumpRequested_ = false;
    blueJumpRequested_ = false;

    // Update red player movement direction (WASD)
    if (redRight_ != KEY_NULL && IsKeyDown(redRight_)) redMoveDir_.x += 1.0f;
    if (redLeft_ != KEY_NULL && IsKeyDown(redLeft_)) redMoveDir_.x -= 1.0f;
    if (redUp_ != KEY_NULL && IsKeyDown(redUp_)) redMoveDir_.z -= 1.0f;
    if (redDown_ != KEY_NULL && IsKeyDown(redDown_)) redMoveDir_.z += 1.0f;
    if (IsKeyPressed(KEY_SPACE)) redJumpRequested_ = true;
    
    // Update blue player movement direction (arrow keys)
    if (blueRight_ != KEY_NULL && IsKeyDown(blueRight_)) blueMoveDir_.x += 1.0f;
    if (blueLeft_ != KEY_NULL && IsKeyDown(blueLeft_)) blueMoveDir_.x -= 1.0f;
    if (blueUp_ != KEY_NULL && IsKeyDown(blueUp_)) blueMoveDir_.z -= 1.0f;
    if (blueDown_ != KEY_NULL && IsKeyDown(blueDown_)) blueMoveDir_.z += 1.0f;
    if (IsKeyPressed(KEY_M)) blueJumpRequested_ = true;
}

// Implementation of camera control methods
void GameScene::panCamera(float yawDelta, float pitchDelta) {
    // Convert input from degrees to radians, since raylib functions expect radians
    float yawRadians = yawDelta * DEG2RAD;
    float pitchRadians = pitchDelta * DEG2RAD;
    
    // Calculate forward vector
    Vector3 forward = Vector3Subtract(camera_.target, camera_.position);
    forward = Vector3Normalize(forward);
    
    // Calculate right vector
    Vector3 right = { forward.z, 0, -forward.x };
    right = Vector3Normalize(right);
    
    // Apply yaw rotation (around global up-vector)
    float cosYaw = cosf(yawRadians);
    float sinYaw = sinf(yawRadians);
    
    Vector3 newForward;
    newForward.x = forward.x * cosYaw - forward.z * sinYaw;
    newForward.y = forward.y;
    newForward.z = forward.x * sinYaw + forward.z * cosYaw;
    
    // Apply pitch rotation (around local right-vector)
    float cosPitch = cosf(pitchRadians);
    float sinPitch = sinf(pitchRadians);
    
    Vector3 finalForward;
    finalForward.x = newForward.x;
    finalForward.y = newForward.y * cosPitch - newForward.z * sinPitch;
    finalForward.z = newForward.y * sinPitch + newForward.z * cosPitch;
    
    // Update camera target
    float distance = Vector3Length(Vector3Subtract(camera_.target, camera_.position));
    camera_.target = Vector3Add(camera_.position, Vector3Scale(finalForward, distance));
}

void GameScene::moveCameraUp(float amount) {
    // Move both camera position and target along the world up vector
    Vector3 up = { 0.0f, 1.0f, 0.0f };
    Vector3 offset = Vector3Scale(up, amount);
    
    camera_.position = Vector3Add(camera_.position, offset);
    camera_.target = Vector3Add(camera_.target, offset);
}

void GameScene::zoomCamera(float zoomAmount) {
    // Change field of view to zoom in or out
    camera_.fovy += zoomAmount;
    
    // Clamp FOV to reasonable limits
    if (camera_.fovy < 10.0f) camera_.fovy = 10.0f;
    if (camera_.fovy > 120.0f) camera_.fovy = 120.0f;
}

void GameScene::render() {
    // Draw white background for the viewport
    DrawRectangle(0, 0, bounds_.width, bounds_.height, RAYWHITE);
    // Draw viewport label
    DrawText(label_, 10, 5, 20, BLACK);
    BeginMode3D(camera_);
    
    // Draw both players
    if(redPlayer_) redPlayer_->draw();
    if(bluePlayer_) bluePlayer_->draw();
    
    DrawGrid(10, 1.0f);
    EndMode3D();
}

}} // namespace netcode::visualization
