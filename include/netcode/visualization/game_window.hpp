#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <optional>
#include <vector>

namespace netcode {
namespace visualization {

class GameWindow {
public:
    GameWindow(const std::string& title, unsigned int width = 800, unsigned int height = 600);
    ~GameWindow();

    void run();
    void set_status_text(const std::string& text);
    void add_network_message(const std::string& message);

private:
    void processEvents();
    void update();
    void render();
    void update_network_messages_display();

    sf::RenderWindow window_;
    sf::CircleShape player_;
    float moveSpeed_ = 5.0f;
    bool running_ = false;

    // Required for text rendering
    sf::Font font_;
    sf::Text statusText_;
    sf::Text inputLabel_;
    sf::Text buttonLabel_;

    std::vector<std::string> network_messages_;
    std::vector<sf::Text> network_message_texts_;
    int max::network_messages_ = 5;
};

}} // namespace netcode::visualization 