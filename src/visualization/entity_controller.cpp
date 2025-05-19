#include "netcode/visualization/entity_controller.hpp"

namespace netcode {
namespace visualization {

LocalEntityController::LocalEntityController() : player_(PlayerType::RED_PLAYER, {0, 0, 0}, RED) {}

void LocalEntityController::updatePlayerOne(const Vector3& movement) {
    player_.move(movement);
}

void LocalEntityController::update() {
    player_.update();
}

}} // namespace netcode::visualization 