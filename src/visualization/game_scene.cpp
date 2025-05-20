#include "netcode/visualization/game_scene.hpp"
#include "rlgl.h"

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

void GameScene::render() {
    // Draw white background for the viewport
    DrawRectangle(0, 0, bounds_.width, bounds_.height, RAYWHITE);
    // Draw viewport label
    DrawText(label_, 10, 5, 20, BLACK);
    camera_.position = {0.0f, 10.0f, 2.0f};
    BeginMode3D(camera_);
    
    // Draw both players
    if(redPlayer_) redPlayer_->draw();
    if(bluePlayer_) bluePlayer_->draw();
    
    DrawGrid(10, 1.0f);
    EndMode3D();
}

}} // namespace netcode::visualization
