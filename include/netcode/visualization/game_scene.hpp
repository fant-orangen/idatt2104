#pragma once

#include "raylib.h"
#include "netcode/visualization/player.hpp"
#include "netcode/math/my_vec3.hpp"
#include <memory>

namespace netcode {
namespace visualization {

// Forward declaration
class ConcreteSettings;

/**
 * @struct Viewport
 * @brief Represents a viewport with bounds, a camera, and a label.
 * @details Used to define a specific rendering area and its associated camera.
 */
struct Viewport {
    Rectangle bounds;
    Camera3D camera;
    const char* label;
};

/**
 * @struct FramebufferRect
 * @brief Represents a rectangle in framebuffer coordinates (pixels).
 * @details Used for converting logical rectangles to framebuffer space.
 */
struct FramebufferRect {
    int x, y, width, height;
};

FramebufferRect toFramebufferRect(const Rectangle& logicalRect);

/**
 * @class GameScene
 * @brief Manages the game scene with multiple viewports.
 * @details Handles rendering, updating, and input processing for a 3D game scene.
 * Provides methods for camera control, player management, and texture handling.
 */
class GameScene {
public:
    /**
     * @brief Constructs a GameScene with the specified viewport dimensions and label.
     * @param viewportWidth Width of the viewport.
     * @param viewportHeight Height of the viewport.
     * @param x X-coordinate of the viewport's position.
     * @param y Y-coordinate of the viewport's position.
     * @param label Label for the viewport.
     * @param settings Reference to the concrete settings (optional, can be set later)
     */
    GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label, ConcreteSettings* settings = nullptr);
    ~GameScene();
    
    /**
     * @brief Set the settings reference
     * 
     * @param settings Pointer to the concrete settings
     */
    void setSettings(ConcreteSettings* settings);
    
    /**
     * @brief Renders the game scene.
     * @details Draws all objects, players, and textures in the scene.
     */
    void render();
    /**
     * @brief Processes input for the game scene.
     * @details Handles player movement, camera control, and other input events.
     */
    void handleInput();

    // Getters for player instances
    std::shared_ptr<Player> getRedPlayer() const { return redPlayer_; }
    std::shared_ptr<Player> getBluePlayer() const { return bluePlayer_; }

    // Getters for movement directions - these remain as Vector3 for the visualization layer
    Vector3 getRedMovementDirection() const { return redMoveDir_; }
    Vector3 getBlueMovementDirection() const { return blueMoveDir_; }
    bool getRedJumpRequested() const { return redJumpRequested_; }
    bool getBlueJumpRequested() const { return blueJumpRequested_; }

    // Camera control methods
    /**
     * @brief Pans the camera by adjusting yaw and pitch.
     * @param yawDelta Change in yaw (horizontal rotation).
     * @param pitchDelta Change in pitch (vertical rotation).
     */
    void panCamera(float yawDelta, float pitchDelta);
    /**
     * @brief Moves the camera up or down.
     * @param amount Distance to move the camera.
     */
    void moveCameraUp(float amount);
    /**
     * @brief Moves the camera left or right.
     * @param amount Distance to move the camera.
     */
    void moveCameraRight(float amount);
    /**
     * @brief Zooms the camera in or out.
     * @param zoomAmount Amount to zoom the camera.
     */
    void zoomCamera(float zoomAmount);

    // Texture control
    /**
     * @brief Toggles texture usage for rendering.
     * @param use Whether to use textures.
     */
    void setUseTexture(bool use) { USE_TEXTURE = use; }
    /**
     * @brief Checks if textures are being used for rendering.
     * @return True if textures are enabled, false otherwise.
     */
    bool getUseTexture() const { return USE_TEXTURE; }

private:
    Rectangle bounds_;
    const char* label_;
    Camera3D camera_;
    std::shared_ptr<Player> redPlayer_;   // Red player (WASD)
    std::shared_ptr<Player> bluePlayer_;  // Blue player (arrow keys)
    
    // Settings reference
    ConcreteSettings* settings_;
    
    // Texture control
    bool USE_TEXTURE = false;
    
    // Texture support
    Texture2D groundTexture_ = LoadTexture("../assets/grass/textures/grass2.jpg"); 
    Model groundModel_;
    bool groundTextureLoaded_ = false;
    
    // Movement direction vectors
    Vector3 redMoveDir_ = {0.0f, 0.0f, 0.0f};
    Vector3 blueMoveDir_ = {0.0f, 0.0f, 0.0f};
    bool redJumpRequested_ = false;
    bool blueJumpRequested_ = false;
};

}} // namespace netcode::visualization
