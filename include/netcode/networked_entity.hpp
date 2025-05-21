#pragma once

#include "raylib.h"
#include <cstdint>

namespace netcode {

/**
 * An interface representing any entity that can be networked
 * across the client-server architecture
 */
class NetworkedEntity {
public:
    virtual ~NetworkedEntity() = default;

    // Movement and physics methods
    virtual void move(const Vector3& direction) = 0;
    virtual void update() = 0;
    virtual void jump() = 0;
    

    // Position methods
    virtual Vector3 getPosition() const = 0;
    virtual void setPosition(const Vector3& pos) = 0;

    // Identity methods
    virtual uint32_t getId() const = 0;
    virtual float getMoveSpeed() const = 0;
};

} // namespace netcode
