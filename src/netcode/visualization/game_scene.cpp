#include "netcode/visualization/game_scene.hpp"
#include "netcode/visualization/network_utility.hpp"
#include "netcode/visualization/concrete_settings.hpp"
#include "rlgl.h"
#include "raymath.h"

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

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label, ConcreteSettings* settings)
    : label_(label), settings_(settings) {
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
    redPlayer_ = std::make_shared<Player>(PlayerType::RED_PLAYER, toMyVec3(Vector3{-2.0f, 1.0f, 0.0f}), RED);
    bluePlayer_ = std::make_shared<Player>(PlayerType::BLUE_PLAYER, toMyVec3(Vector3{2.0f, 1.0f, 0.0f}), BLUE);

    if (groundTexture_.id > 0) {
        // Create a plane with more segments for better texture mapping
        Mesh mesh = GenMeshPlane(100.0f, 100.0f, 10, 10);
        
        // Create a material for the ground
        groundModel_ = LoadModelFromMesh(mesh);
        
        // Set texture and material properties
        groundModel_.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTexture_;
        groundTextureLoaded_ = true;
        
        // Set texture parameters
        SetTextureWrap(groundTexture_, TEXTURE_WRAP_REPEAT);
        SetTextureFilter(groundTexture_, TEXTURE_FILTER_TRILINEAR);
        
        // Scale the texture coordinates to make the texture repeat
        // A smaller scale value makes the texture repeat more
        float textureScale = 0.05f;
        for (int i = 0; i < groundModel_.meshCount; i++) {
            groundModel_.meshes[i].texcoords[0] = 0.0f;
            groundModel_.meshes[i].texcoords[1] = 0.0f;
            groundModel_.meshes[i].texcoords[2] = textureScale;
            groundModel_.meshes[i].texcoords[3] = 0.0f;
            groundModel_.meshes[i].texcoords[4] = textureScale;
            groundModel_.meshes[i].texcoords[5] = textureScale;
            groundModel_.meshes[i].texcoords[6] = 0.0f;
            groundModel_.meshes[i].texcoords[7] = textureScale;
            
            // Upload the modified mesh data
            UpdateMeshBuffer(groundModel_.meshes[i], 1, groundModel_.meshes[i].texcoords, groundModel_.meshes[i].vertexCount*2*sizeof(float), 0);
        }
    }
}

void GameScene::setSettings(ConcreteSettings* settings) {
    settings_ = settings;
}

void GameScene::handleInput() {
    // Reset movement directions and jump requests
    redMoveDir_ = {0.0f, 0.0f, 0.0f};
    blueMoveDir_ = {0.0f, 0.0f, 0.0f};
    redJumpRequested_ = false;
    blueJumpRequested_ = false;

    // Update red player movement direction using settings if available
    if (settings_) {
        if (IsKeyDown(settings_->getPlayer1Right())) redMoveDir_.x += 1.0f;
        if (IsKeyDown(settings_->getPlayer1Left())) redMoveDir_.x -= 1.0f;
        if (IsKeyDown(settings_->getPlayer1Up())) redMoveDir_.z -= 1.0f;
        if (IsKeyDown(settings_->getPlayer1Down())) redMoveDir_.z += 1.0f;
        if (IsKeyPressed(settings_->getPlayer1Jump())) redJumpRequested_ = true;
        
        // Update blue player movement direction
        if (IsKeyDown(settings_->getPlayer2Right())) blueMoveDir_.x += 1.0f;
        if (IsKeyDown(settings_->getPlayer2Left())) blueMoveDir_.x -= 1.0f;
        if (IsKeyDown(settings_->getPlayer2Up())) blueMoveDir_.z -= 1.0f;
        if (IsKeyDown(settings_->getPlayer2Down())) blueMoveDir_.z += 1.0f;
        if (IsKeyPressed(settings_->getPlayer2Jump())) blueJumpRequested_ = true;
    } else {
        // Fallback to default keys if no settings available
        if (IsKeyDown(KEY_D)) redMoveDir_.x += 1.0f;
        if (IsKeyDown(KEY_A)) redMoveDir_.x -= 1.0f;
        if (IsKeyDown(KEY_W)) redMoveDir_.z -= 1.0f;
        if (IsKeyDown(KEY_S)) redMoveDir_.z += 1.0f;
        if (IsKeyPressed(KEY_SPACE)) redJumpRequested_ = true;
        
        if (IsKeyDown(KEY_L)) blueMoveDir_.x += 1.0f;
        if (IsKeyDown(KEY_J)) blueMoveDir_.x -= 1.0f;
        if (IsKeyDown(KEY_I)) blueMoveDir_.z -= 1.0f;
        if (IsKeyDown(KEY_K)) blueMoveDir_.z += 1.0f;
        if (IsKeyPressed(KEY_M)) blueJumpRequested_ = true;
    }
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
    
    // Apply yaw rotation
    float cosYaw = cosf(yawRadians);
    float sinYaw = sinf(yawRadians);
    
    Vector3 newForward;
    newForward.x = forward.x * cosYaw - forward.z * sinYaw;
    newForward.y = forward.y;
    newForward.z = forward.x * sinYaw + forward.z * cosYaw;
    
    // Apply pitch rotation
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
    ClearBackground(RAYWHITE);
    // Draw viewport label
    DrawText(label_, 10, 5, 20, BLACK);
    BeginMode3D(camera_);
    
    // Draw both players
    if(redPlayer_) redPlayer_->draw();
    if(bluePlayer_) bluePlayer_->draw();
    
    // Draw either textured ground or grid based on settings
    if (settings_ && settings_->useTexturedGround() && groundTextureLoaded_) {
        DrawModel(groundModel_, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    } else {
        DrawGrid(10, 1.0f);
    }
    EndMode3D();
}

}} // namespace netcode::visualization
