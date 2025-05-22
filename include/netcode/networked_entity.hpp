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

    /**
     * @brief Move the entity in the given direction
     * @param direction The direction to move in
     */
    virtual void move(const Vector3& direction) = 0;

    /**
     * @brief Update the entity's position and velocity
     */
    virtual void update() = 0;

    /**
     * @brief Make the entity jump
     */
    virtual void jump() = 0;
    

    // Position methods

    /**
     * @brief Get the entity's position
     * @return The entity's position
     */
    virtual Vector3 getPosition() const = 0;

    /**
     * @brief Set the entity's position
     * @param pos The new position
     */
    virtual void setPosition(const Vector3& pos) = 0;

    // Identity methods

    /**
     * @brief Get the entity's ID
     * @return The entity's ID
     */
    virtual uint32_t getId() const = 0;
    
    /**
     * @brief Get the entity's move speed
     * @return The entity's move speed
     */
    virtual float getMoveSpeed() const = 0;
};

} // namespace netcode
