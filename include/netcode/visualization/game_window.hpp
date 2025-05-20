#pragma once

#include <memory>
#include <raylib.h>
#include "netcode/visualization/game_scene.hpp"
#include "netcode/visualization/network_utility.hpp"

namespace netcode {
namespace visualization {

/**
 * @class GameWindow
 * @brief Manages the game window and main application loop
 * 
 * Handles window creation, event processing, and the main game loop.
 * Acts as a container for the GameScene which handles the actual game rendering.
 */
class GameWindow {
public:
    /**
     * @brief Constructs a GameWindow with the specified title and dimensions
     * @param title The window title
     * @param width The window width in pixels (default: 800)
     * @param height The window height in pixels (default: 600)
     */
    GameWindow(const char* title, int width = 800, int height = 600);
    
    /**
     * @brief Destructor - closes the window
     */
    ~GameWindow();

    /**
     * @brief Starts and runs the main game loop
     * 
     * Continues running until the window is closed or the application
     * is terminated.
     */
    void run();
    void set_status_text(const std::string& text);
    void add_network_message(const std::string& message);

private:
    /**
     * @brief Processes window and input events
     */
    void processEvents();
    
    /**
     * @brief Updates game logic
     */
    void update();
    
    /**
     * @brief Renders the current frame
     */
    void render();
    void update_network_messages_display();
    
    /**
     * @brief Handle camera control input
     */
    void handleCameraInput();

    void createScenes(int width, int height);

    bool running_;  ///< Flag indicating if the game loop should continue running
    
    std::unique_ptr<GameScene> scene1_;
    std::unique_ptr<GameScene> scene2_;
    std::unique_ptr<GameScene> scene3_;
    RenderTexture2D rt1_;
    RenderTexture2D rt2_;
    RenderTexture2D rt3_;
    std::unique_ptr<NetworkUtility> network_;

    // Camera control variables
    GameScene* activeSceneForCamera_; // Currently active scene for camera control
    int activeSceneIndex_; // Index of active scene (1, 2, or 3)
    Vector2 prevMousePos_; // Previous mouse position for delta calculation
    bool mouseRightPressed_; // Track mouse right button state
    
    // Camera control parameters
    const float CAMERA_MOVE_SPEED = 0.3f;
    const float CAMERA_PAN_SPEED = 0.2f;
    const float CAMERA_ZOOM_SPEED = 2.0f;
};

}} // namespace netcode::visualization 