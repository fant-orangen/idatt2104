#include "netcode/visualization/concrete_settings.hpp"

namespace netcode {
namespace visualization {

ConcreteSettings::ConcreteSettings() {
    // Player 1 (Red) Controls
    player1Up_ = KEY_W;
    player1Down_ = KEY_S;
    player1Left_ = KEY_A;
    player1Right_ = KEY_D;
    player1Jump_ = KEY_SPACE;
    
    // Player 2 (Blue) Controls
    player2Up_ = KEY_I;
    player2Down_ = KEY_K;
    player2Left_ = KEY_J;
    player2Right_ = KEY_L;
    player2Jump_ = KEY_M;
    
    // Camera Controls
    cameraUp_ = KEY_T;
    cameraDown_ = KEY_G;
    cameraLeft_ = KEY_H;
    cameraRight_ = KEY_F;
    cameraZoomIn_ = KEY_EQUAL;
    cameraZoomOut_ = KEY_MINUS;
    
    // Visualization Settings
    useTexturedGround_ = true;
    
    // Network Delay Settings (in milliseconds)
    clientToServerDelay_ = 10;
    serverToClientDelay_ = 50;
    
    // Reconciliation Settings
    enablePrediction_ = false;
    enableInterpolation_ = false;
}

int ConcreteSettings::getClientToServerDelay() const {
    return clientToServerDelay_;
}

int ConcreteSettings::getServerToClientDelay() const {
    return serverToClientDelay_;
}

bool ConcreteSettings::isPredictionEnabled() const {
    return enablePrediction_;
}

bool ConcreteSettings::isInterpolationEnabled() const {
    return enableInterpolation_;
}

}} // namespace netcode::visualization 