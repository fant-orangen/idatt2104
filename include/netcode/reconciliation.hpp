#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/snapshot.hpp"
#include "netcode/prediction.hpp"
#include <memory>
#include <functional>

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
    
private:
    PredictionSystem& predictionSystem_;
    float reconciliationThreshold_ = 0.5f; // Minimum difference to trigger reconciliation
    
    // Callback for when reconciliation happens (entityId, serverPos, clientPos)
    std::function<void(uint32_t, const netcode::math::MyVec3&, const netcode::math::MyVec3&)> reconciliationCallback_;
    
    /**
     * @brief Reapply inputs after a server correction
     * @param entity The entity to reapply inputs for
     * @param serverSequence The sequence number from the server
     */
    void reapplyInputs(
        std::shared_ptr<NetworkedEntity> entity,
        uint32_t serverSequence
    );
};

} // namespace netcode 