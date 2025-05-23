#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include <chrono>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>

namespace netcode {

/**
 * @brief Represents a snapshot of an entity's state at a point in time
 */
struct EntitySnapshot {
    uint32_t entityId;
    netcode::math::MyVec3 position;
    netcode::math::MyVec3 velocity;
    bool isJumping;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t sequenceNumber; // Used for reconciliation
    
    // Comparison operator for sorting
    bool operator<(const EntitySnapshot& other) const {
        return sequenceNumber < other.sequenceNumber;
    }
};

/**
 * @brief Input state for a player at a point in time
 */
struct InputSnapshot {
    uint32_t playerId;
    netcode::math::MyVec3 movement;
    bool isJumping;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t sequenceNumber;
    
    bool operator<(const InputSnapshot& other) const {
        return sequenceNumber < other.sequenceNumber;
    }
};

/**
 * @brief Class to manage state snapshots for entities
 */
class SnapshotManager {
public:
    /**
     * @brief Store a snapshot of an entity's state
     * @param snapshot The entity snapshot to store
     */
    void storeEntitySnapshot(const EntitySnapshot& snapshot);
    
    /**
     * @brief Store an input snapshot
     * @param input The input snapshot to store
     */
    void storeInputSnapshot(const InputSnapshot& input);
    
    /**
     * @brief Get the most recent snapshot for an entity
     * @param entityId The entity ID to get the snapshot for
     * @return The most recent snapshot, or an empty snapshot if none exists
     */
    EntitySnapshot getLatestEntitySnapshot(uint32_t entityId) const;
    
    /**
     * @brief Get all snapshots for an entity after a specified sequence number
     * @param entityId The entity ID to get snapshots for
     * @param afterSequence The sequence number to start from
     * @return A vector of snapshots
     */
    std::vector<EntitySnapshot> getEntitySnapshotsAfter(uint32_t entityId, uint32_t afterSequence) const;
    
    /**
     * @brief Get all input snapshots for a player after a specified sequence number
     * @param playerId The player ID to get inputs for
     * @param afterSequence The sequence number to start from
     * @return A vector of input snapshots
     */
    std::vector<InputSnapshot> getInputSnapshotsAfter(uint32_t playerId, uint32_t afterSequence) const;
    
    /**
     * @brief Get an entity by its ID
     * @param entityId The entity ID to retrieve
     * @return A shared pointer to the entity, or nullptr if not found
     */
    std::shared_ptr<NetworkedEntity> getEntity(uint32_t entityId) const;
    
    /**
     * @brief Register an entity with the snapshot manager
     * @param entityId The entity ID
     * @param entity The entity to register
     */
    void registerEntity(uint32_t entityId, std::shared_ptr<NetworkedEntity> entity);
    
    /**
     * @brief Remove old snapshots to prevent memory growth
     * @param maxAge Maximum age of snapshots to keep in milliseconds
     */
    void pruneOldSnapshots(uint64_t maxAge);
    
private:
    // Maps entity ID to a vector of snapshots
    std::map<uint32_t, std::vector<EntitySnapshot>> entitySnapshots_;
    
    // Maps player ID to a vector of input snapshots
    std::map<uint32_t, std::vector<InputSnapshot>> inputSnapshots_;
    
    // Maps entity ID to entity instances (weak references to avoid ownership issues)
    mutable std::map<uint32_t, std::weak_ptr<NetworkedEntity>> entities_;
};

} // namespace netcode 