#include "netcode/visualization/game_scene.hpp"

namespace netcode {
namespace visualization {

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label)
    : label_(label) {
    bounds_ = {x, y, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
}

void GameScene::render() {
    // Set up the viewport for this scene
    BeginScissorMode(bounds_.x, bounds_.y, bounds_.width, bounds_.height);
    
    // Draw light gray background for the viewport
    DrawRectangle(bounds_.x, bounds_.y, bounds_.width, bounds_.height, LIGHTGRAY);
    
    // Draw viewport label
    DrawText(label_, bounds_.x + 10, bounds_.y + 5, 20, BLACK);
    
    // Draw coordinate axes
    DrawLine(bounds_.x + 10, bounds_.y + 30, bounds_.x + 50, bounds_.y + 30, RED);    // X axis
    DrawLine(bounds_.x + 10, bounds_.y + 30, bounds_.x + 10, bounds_.y + 70, GREEN);  // Y axis
    
    EndScissorMode();
}

}} // namespace netcode::visualization
