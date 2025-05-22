#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/snapshot.hpp"
#include <memory>
#include <map>
#include <chrono>

namespace netcode {

/**
 * @brief Configuration for entity interpolation
 */
struct InterpolationConfig {
    // Time in milliseconds to interpolate between positions
    uint32_t interpolationDelay = 100;
    
    // History buffer size for interpolation
    uint32_t historySize = 10;
    
    // Maximum allowed position jump before snapping instead of interpolating
    float maxInterpolationDistance = 5.0f;
};

/**
 * @brief Handles interpolation for networked entities
 * 
 * This class manages smooth movement between received network updates,
 * particularly for other players/entities to create fluid motion.
 */
class InterpolationSystem {
public:
    /**
     * @brief Constructor
     * @param snapshotManager Reference to the snapshot manager
     * @param config Configuration for interpolation
     */
    InterpolationSystem(SnapshotManager& snapshotManager, const InterpolationConfig& config = InterpolationConfig());
    
    /**
     * @brief Update an entity's position using interpolation
     * 
     * @param entity The entity to update
     * @param deltaTime Time since last update in seconds
     */
    void updateEntity(std::shared_ptr<NetworkedEntity> entity, float deltaTime);
    
    /**
     * @brief Record a new position for an entity for future interpolation
     * 
     * @param entityId ID of the entity
     * @param position New position
     * @param timestamp Time when this position was valid
     */
    void recordEntityPosition(
        uint32_t entityId,
        const netcode::math::MyVec3& position,
        std::chrono::steady_clock::time_point timestamp
    );
    
    /**
     * @brief Set the interpolation configuration
     * @param config New configuration
     */
    void setConfig(const InterpolationConfig& config);
    
    /**
     * @brief Get the current interpolation configuration
     * @return The current configuration
     */
    const InterpolationConfig& getConfig() const;
    
    /**
     * @brief Reset the interpolation system's state
     */
    void reset();
    
private:
    SnapshotManager& snapshotManager_;
    InterpolationConfig config_;
    
    // Maps entity ID -> render time (offset from actual time)
    std::map<uint32_t, std::chrono::steady_clock::time_point> renderTimes_;
    
    // Maps entity ID -> current interpolation target
    std::map<uint32_t, EntitySnapshot> interpolationTargets_;
    
    /**
     * @brief Find the appropriate snapshots to interpolate between
     * 
     * @param entityId ID of the entity
     * @param renderTime The time to find snapshots for
     * @param out_start Output parameter for the starting snapshot
     * @param out_end Output parameter for the ending snapshot
     * @param out_t Output parameter for interpolation factor [0-1]
     * @return True if snapshots were found, false otherwise
     */
    bool findInterpolationSnapshots(
        uint32_t entityId,
        std::chrono::steady_clock::time_point renderTime,
        EntitySnapshot& out_start,
        EntitySnapshot& out_end,
        float& out_t
    );
};

} // namespace netcode 