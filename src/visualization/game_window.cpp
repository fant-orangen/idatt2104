#include "netcode/visualization/game_window.hpp"
#include <iostream>
#include <optional>

namespace netcode {
namespace visualization {

GameWindow::GameWindow(const std::string& title, unsigned int width, unsigned int height)
    : window_(sf::VideoMode(sf::Vector2u(width, height)), title),
      statusText_(font_),
      inputLabel_(font_),
      buttonLabel_(font_)
{
    // Initialize player
    player_.setRadius(20.f);
    player_.setFillColor(sf::Color::Blue);
    player_.setOrigin(sf::Vector2f(20.f, 20.f));  // Center the circle's origin
    player_.setPosition(sf::Vector2f(width / 2.f, height / 2.f));  // Center in window
    
    // Load font
    if (!font_.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        std::cerr << "Failed to load font" << std::endl;
        return;
    }

    // Initialize text objects
    statusText_.setString("");
    statusText_.setCharacterSize(24);
    
    inputLabel_.setString("");
    inputLabel_.setCharacterSize(16);
    
    buttonLabel_.setString("");
    buttonLabel_.setCharacterSize(16);
    
    running_ = true;
}

GameWindow::~GameWindow() {
}

void GameWindow::run() {
    while (running_ && window_.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void GameWindow::processEvents() {
    std::optional<sf::Event> event;
    while ((event = window_.pollEvent())) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
            running_ = false;
        }
    }
}

void GameWindow::update() {
    sf::Vector2f movement(0.f, 0.f);
    
    // Real-time input handling outside the event loop
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
        movement.x -= moveSpeed_;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
        movement.x += moveSpeed_;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {
        movement.y -= moveSpeed_;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
        movement.y += moveSpeed_;
    }
    
    player_.move(movement);
}

void GameWindow::render() {
    window_.clear(sf::Color(30, 30, 30));  // Dark gray background
    window_.draw(player_);
    window_.display();
}

}} // namespace netcode::visualization 