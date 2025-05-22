#pragma once

#include "netcode/math/my_vec3.hpp"
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
    virtual void move(const netcode::math::MyVec3& direction) = 0;

    /**
     * @brief Update the entity's position and velocity
     */
    virtual void update() = 0;

    /**
     * @brief Make the entity jump
     */
    virtual void jump() = 0;
    
    /**
     * @brief Update the entity's render position for smooth visual transitions
     * @param deltaTime Time since last update in seconds
     */
    virtual void updateRenderPosition(float deltaTime) = 0;
    
    /**
     * @brief Snap the entity's simulation state to match server data
     * @param position The authoritative position from the server
     * @param isJumping The jumping state from the server
     * @param velocityY The Y velocity component from the server (optional)
     */
    virtual void snapSimulationState(
        const netcode::math::MyVec3& position, 
        bool isJumping = false, 
        float velocityY = 0.0f) = 0;
    
    /**
     * @brief Initiate a visual blend from current render position to simulation position
     */
    virtual void initiateVisualBlend() = 0;

    // Position methods

    /**
     * @brief Get the entity's simulation position (used for physics)
     * @return The entity's simulation position
     */
    virtual netcode::math::MyVec3 getPosition() const = 0;
    
    /**
     * @brief Get the entity's render position (used for display)
     * @return The entity's render position
     */
    virtual netcode::math::MyVec3 getRenderPosition() const = 0;

    /**
     * @brief Set the entity's simulation position
     * @param pos The new position
     */
    virtual void setPosition(const netcode::math::MyVec3& pos) = 0;
    
    /**
     * @brief Get the entity's velocity
     * @return The entity's velocity vector
     */
    virtual netcode::math::MyVec3 getVelocity() const = 0;

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
