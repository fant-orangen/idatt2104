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

GameScene::~GameScene() {
    if (groundTextureLoaded_) {
        UnloadTexture(groundTexture_);
        UnloadModel(groundModel_);
    }
}

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label)
    : label_(label) {
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

    if (groundTexture_.id > 0) {
        // Create a much larger plane with more subdivisions for better texture mapping
        Mesh mesh = GenMeshPlane(2000.0f, 2000.0f, 50, 50);  // Larger plane with more subdivisions
        groundModel_ = LoadModelFromMesh(mesh);
        groundModel_.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTexture_;
        groundTextureLoaded_ = true;
        SetTextureWrap(groundTexture_, TEXTURE_WRAP_REPEAT);
    }
}

void GameScene::handleInput() {
    // Reset movement directions and jump requests
    redMoveDir_ = {0.0f, 0.0f, 0.0f};
    blueMoveDir_ = {0.0f, 0.0f, 0.0f};
    redJumpRequested_ = false;
    blueJumpRequested_ = false;

    // Update red player movement direction
    if (IsKeyDown(settings::PLAYER1_RIGHT)) redMoveDir_.x += 1.0f;
    if (IsKeyDown(settings::PLAYER1_LEFT)) redMoveDir_.x -= 1.0f;
    if (IsKeyDown(settings::PLAYER1_UP)) redMoveDir_.z -= 1.0f;
    if (IsKeyDown(settings::PLAYER1_DOWN)) redMoveDir_.z += 1.0f;
    if (IsKeyPressed(settings::PLAYER1_JUMP)) redJumpRequested_ = true;
    
    // Update blue player movement direction
    if (IsKeyDown(settings::PLAYER2_RIGHT)) blueMoveDir_.x += 1.0f;
    if (IsKeyDown(settings::PLAYER2_LEFT)) blueMoveDir_.x -= 1.0f;
    if (IsKeyDown(settings::PLAYER2_UP)) blueMoveDir_.z -= 1.0f;
    if (IsKeyDown(settings::PLAYER2_DOWN)) blueMoveDir_.z += 1.0f;
    if (IsKeyPressed(settings::PLAYER2_JUMP)) blueJumpRequested_ = true;
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

void GameScene::moveCameraRight(float amount) {
    // Calculate the right vector based on the camera's forward direction
    Vector3 forward = Vector3Subtract(camera_.target, camera_.position);
    forward = Vector3Normalize(forward);
    
    // Right vector is perpendicular to forward in the XZ plane
    Vector3 right = { forward.z, 0.0f, -forward.x };
    right = Vector3Normalize(right);
    
    // Scale the right vector by the movement amount
    Vector3 offset = Vector3Scale(right, amount);
    
    // Move both camera position and target to maintain the same view direction
    camera_.position = Vector3Add(camera_.position, offset);
    camera_.target = Vector3Add(camera_.target, offset);
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
    
    // Draw either textured ground or grid based on USE_TEXTURE flag
    if (settings::USE_TEXTURED_GROUND && groundTextureLoaded_) {
        DrawModel(groundModel_, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    } else {
        DrawGrid(10, 1.0f);
    }
    EndMode3D();
}

}} // namespace netcode::visualization
