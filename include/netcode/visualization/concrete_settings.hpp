#pragma once

#include "netcode/settings.hpp"
#include "raylib.h"

namespace netcode {
namespace visualization {

/**
 * @brief Concrete implementation of ISettings for the visualization layer
 * This implementation provides access to all the visualization-specific settings
 * while also implementing the general settings interface
 */
class ConcreteSettings : public netcode::ISettings {
public:
    ConcreteSettings();
    virtual ~ConcreteSettings() = default;
    
    // ISettings interface implementation
    int getClientToServerDelay() const override;
    int getServerToClientDelay() const override;
    bool isPredictionEnabled() const override;
    bool isInterpolationEnabled() const override;
    
    // Visualization-specific settings access
    // Player 1 (Red) Controls
    KeyboardKey getPlayer1Up() const { return player1Up_; }
    KeyboardKey getPlayer1Down() const { return player1Down_; }
    KeyboardKey getPlayer1Left() const { return player1Left_; }
    KeyboardKey getPlayer1Right() const { return player1Right_; }
    KeyboardKey getPlayer1Jump() const { return player1Jump_; }
    
    // Player 2 (Blue) Controls
    KeyboardKey getPlayer2Up() const { return player2Up_; }
    KeyboardKey getPlayer2Down() const { return player2Down_; }
    KeyboardKey getPlayer2Left() const { return player2Left_; }
    KeyboardKey getPlayer2Right() const { return player2Right_; }
    KeyboardKey getPlayer2Jump() const { return player2Jump_; }
    
    // Camera Controls
    KeyboardKey getCameraUp() const { return cameraUp_; }
    KeyboardKey getCameraDown() const { return cameraDown_; }
    KeyboardKey getCameraLeft() const { return cameraLeft_; }
    KeyboardKey getCameraRight() const { return cameraRight_; }
    KeyboardKey getCameraZoomIn() const { return cameraZoomIn_; }
    KeyboardKey getCameraZoomOut() const { return cameraZoomOut_; }
    
    // Visualization Settings
    bool useTexturedGround() const { return useTexturedGround_; }
    
    // Setters for runtime modification
    void setClientToServerDelay(int delay) { clientToServerDelay_ = delay; }
    void setServerToClientDelay(int delay) { serverToClientDelay_ = delay; }
    void setPredictionEnabled(bool enabled) { enablePrediction_ = enabled; }
    void setInterpolationEnabled(bool enabled) { enableInterpolation_ = enabled; }
    
    // Player control setters
    void setPlayer1Up(KeyboardKey key) { player1Up_ = key; }
    void setPlayer1Down(KeyboardKey key) { player1Down_ = key; }
    void setPlayer1Left(KeyboardKey key) { player1Left_ = key; }
    void setPlayer1Right(KeyboardKey key) { player1Right_ = key; }
    void setPlayer1Jump(KeyboardKey key) { player1Jump_ = key; }
    
    void setPlayer2Up(KeyboardKey key) { player2Up_ = key; }
    void setPlayer2Down(KeyboardKey key) { player2Down_ = key; }
    void setPlayer2Left(KeyboardKey key) { player2Left_ = key; }
    void setPlayer2Right(KeyboardKey key) { player2Right_ = key; }
    void setPlayer2Jump(KeyboardKey key) { player2Jump_ = key; }

private:
    // Player 1 (Red) Controls
    KeyboardKey player1Up_;
    KeyboardKey player1Down_;
    KeyboardKey player1Left_;
    KeyboardKey player1Right_;
    KeyboardKey player1Jump_;
    
    // Player 2 (Blue) Controls
    KeyboardKey player2Up_;
    KeyboardKey player2Down_;
    KeyboardKey player2Left_;
    KeyboardKey player2Right_;
    KeyboardKey player2Jump_;
    
    // Camera Controls
    KeyboardKey cameraUp_;
    KeyboardKey cameraDown_;
    KeyboardKey cameraLeft_;
    KeyboardKey cameraRight_;
    KeyboardKey cameraZoomIn_;
    KeyboardKey cameraZoomOut_;
    
    // Visualization Settings
    bool useTexturedGround_;
    
    // Network Delay Settings (in milliseconds)
    int clientToServerDelay_;
    int serverToClientDelay_;
    
    // Reconciliation Settings
    bool enablePrediction_;
    bool enableInterpolation_;
};

}} // namespace netcode::visualization 