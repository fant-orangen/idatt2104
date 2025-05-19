#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <optional>

namespace netcode {
namespace visualization {

class GameWindow {
public:
    GameWindow(const std::string& title, unsigned int width = 800, unsigned int height = 600);
    ~GameWindow();

    void run();

private:
    void processEvents();
    void update();
    void render();

    sf::RenderWindow window_;
    sf::CircleShape player_;
    float moveSpeed_ = 5.0f;
    bool running_ = false;

    // Required for text rendering
    sf::Font font_;
    sf::Text statusText_;
    sf::Text inputLabel_;
    sf::Text buttonLabel_;
};

}} // namespace netcode::visualization 