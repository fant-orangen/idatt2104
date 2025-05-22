#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/snapshot.hpp"
#include "netcode/prediction.hpp"
#include <memory>
#include <functional>
#include <map>
#include <chrono>

namespace netcode {

/**
 * @brief Handles client-server state reconciliation
 * 
 * This class manages adjusting client state when server corrections arrive
 * and reapplying any pending inputs that happened after the correction.
 */
class ReconciliationSystem {
public:
    /**
     * @brief Constructor
     * @param predictionSystem Reference to the prediction system
     */
    explicit ReconciliationSystem(PredictionSystem& predictionSystem);
    
    /**
     * @brief Process a server update for reconciliation
     * 
     * This method reconciles local state with server authority, then
     * reapplies any pending inputs to maintain responsiveness
     * 
     * @param entity The entity to apply reconciliation to
     * @param serverPosition The position from the server
     * @param serverSequence The sequence number from the server
     * @param serverTimestamp When the server generated this update
     * @return True if reconciliation was needed, false if states already matched
     */
    bool reconcileState(
        std::shared_ptr<NetworkedEntity> entity,
        const netcode::math::MyVec3& serverPosition,
        uint32_t serverSequence,
        std::chrono::steady_clock::time_point serverTimestamp
    );
    
    /**
     * @brief Update reconciliation smoothing for entities
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime);
    
    /**
     * @brief Set the threshold for position discrepancy that triggers reconciliation
     * @param threshold The threshold value
     */
    void setReconciliationThreshold(float threshold);
    
    /**
     * @brief Get the current reconciliation threshold
     * @return The current threshold
     */
    float getReconciliationThreshold() const;
    
    /**
     * @brief Set callback for when significant reconciliation occurs
     * @param callback Function to call when reconciliation happens
     */
    void setReconciliationCallback(std::function<void(uint32_t, const netcode::math::MyVec3&, const netcode::math::MyVec3&)> callback);
    
    /**
     * @brief Set the smoothing factor for reconciliation
     * @param smoothFactor Value between 0 and 1, higher values mean faster correction
     */
    void setSmoothingFactor(float smoothFactor);
    
    /**
     * @brief Reset the reconciliation system's state
     */
    void reset();
    
private:
    struct ReconciliationState {
        netcode::math::MyVec3 targetPosition;
        netcode::math::MyVec3 startPosition;
        bool reconciling = false;
        uint32_t serverSequence = 0; // Server sequence number for this reconciliation
    };

    PredictionSystem& predictionSystem_;
    float reconciliationThreshold_ = 0.5f; // Minimum difference to trigger reconciliation
    float smoothingFactor_ = 10.0f; // Controls how quickly to blend to correct position
    std::map<uint32_t, ReconciliationState> reconciliationStates_;
    
    // Callback for when reconciliation happens (entityId, serverPos, clientPos)
    std::function<void(uint32_t, const netcode::math::MyVec3&, const netcode::math::MyVec3&)> reconciliationCallback_;
    
    /**
     * @brief Reapply inputs after a server correction
     * @param entity The entity to reapply inputs for
     * @param serverSequence The sequence number from the server
     * @param targetPosition The position to reapply from
     */
    void reapplyInputs(
        std::shared_ptr<NetworkedEntity> entity,
        uint32_t serverSequence,
        const netcode::math::MyVec3& targetPosition
    );
};

} // namespace netcode 