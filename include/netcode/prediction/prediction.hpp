#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/snapshot.hpp"
#include <memory>
#include <map>

namespace netcode {

/**
 * @brief Handles client-side prediction for networked entities
 * 
 * This class manages applying inputs locally before server confirmation
 * to make the game feel responsive despite network latency.
 */
class PredictionSystem {
public:
    /**
     * @brief Constructor
     * @param snapshotManager Reference to the snapshot manager for storing states
     */
    explicit PredictionSystem(SnapshotManager& snapshotManager);
    
    /**
     * @brief Apply input prediction for an entity
     * 
     * This method predicts the entity's state based on local input
     * and applies it immediately to provide responsive gameplay
     * 
     * @param entity The entity to apply prediction to
     * @param input The movement input
     * @param isJumping Whether the player is jumping
     * @return The sequence number assigned to this input
     */
    uint32_t applyInputPrediction(
        std::shared_ptr<NetworkedEntity> entity,
        const netcode::math::MyVec3& input,
        bool isJumping
    );
    
    /**
     * @brief Update sequence counter for next prediction
     * @return The next sequence number
     */
    uint32_t getNextSequenceNumber();
    
    /**
     * @brief Get the current sequence number
     * @return The current sequence number
     */
    uint32_t getCurrentSequenceNumber() const;
    
    /**
     * @brief Reset the prediction system's state
     */
    void reset();
    
    /**
     * @brief Get the snapshot manager
     * @return Reference to the snapshot manager
     */
    SnapshotManager& getSnapshotManager();
    
private:
    SnapshotManager& snapshotManager_;
    uint32_t currentSequence_ = 0;
    std::map<uint32_t, std::shared_ptr<NetworkedEntity>> entities_;
};

} // namespace netcode 