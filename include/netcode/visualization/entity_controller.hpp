#pragma once

#include "netcode/visualization/player.hpp"
#include <memory>

namespace netcode {
namespace visualization {

enum class ViewType {
    PLAYER_ONE,
    SERVER,
    PLAYER_TWO
};

class EntityController {
public:
    virtual ~EntityController() = default;
    virtual void updatePlayerOne(const Vector3& movement) = 0;
    virtual Player& getPlayer() = 0;
    virtual void update() = 0;
};

// Local implementation that will later be replaced with networked version
class LocalEntityController : public EntityController {
public:
    LocalEntityController();
    void updatePlayerOne(const Vector3& movement) override;
    Player& getPlayer() override { return player_; }
    void update() override;

private:
    Player player_;
};

}} // namespace netcode::visualization
