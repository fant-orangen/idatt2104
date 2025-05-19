#pragma once

#include "raylib.h"

namespace netcode {
namespace visualization {

class GameWindow {
public:
    GameWindow(const char* title, int width = 800, int height = 600);
    ~GameWindow();

    void run();

private:
    void processInput();
    void update();
    void render();

    bool running_;
    Vector3 playerPosition_;
    const float moveSpeed_ = 0.2f;
    Camera3D camera_;
};

}} // namespace netcode::visualization 