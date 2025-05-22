#include "netcode/reconciliation.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {

ReconciliationSystem::ReconciliationSystem(PredictionSystem& predictionSystem)
    : predictionSystem_(predictionSystem), reconciliationThreshold_(0.5f) {
    LOG_INFO("Reconciliation system initialized", "ReconciliationSystem");
}

bool ReconciliationSystem::reconcileState(
    std::shared_ptr<NetworkedEntity> entity,
    const netcode::math::MyVec3& serverPosition,
    uint32_t serverSequence,
    std::chrono::steady_clock::time_point serverTimestamp) {
    
    if (!entity) {
        LOG_ERROR("Null entity passed to reconciliation system", "ReconciliationSystem");
        return false;
    }
    
    uint32_t entityId = entity->getId();
    netcode::math::MyVec3 clientPosition = entity->getPosition();
    
    // Calculate distance between client and server positions
    float positionDifference = Magnitude(serverPosition - clientPosition);
    
    // If difference is below threshold, no reconciliation needed
    if (positionDifference < reconciliationThreshold_) {
        LOG_DEBUG("No reconciliation needed for entity " + std::to_string(entityId) + 
                 " (diff: " + std::to_string(positionDifference) + ")", "ReconciliationSystem");
        return false;
    }
    
    // Log reconciliation event
    LOG_INFO("Reconciling entity " + std::to_string(entityId) + 
             " (diff: " + std::to_string(positionDifference) + ")", "ReconciliationSystem");
    
    // Store positions for callback
    netcode::math::MyVec3 oldPosition = clientPosition;
    
    // Set entity to server-authoritative position
    entity->setPosition(serverPosition);
    
    // Store this server snapshot
    EntitySnapshot serverSnapshot;
    serverSnapshot.entityId = entityId;
    serverSnapshot.position = serverPosition;
    serverSnapshot.velocity = {0, 0, 0}; // Velocity not tracked in base interface
    serverSnapshot.isJumping = false; // We don't know this from the reconciliation data
    serverSnapshot.timestamp = serverTimestamp;
    serverSnapshot.sequenceNumber = serverSequence;
    predictionSystem_.getSnapshotManager().storeEntitySnapshot(serverSnapshot);
    
    // Reapply any inputs that happened after the server sequence
    reapplyInputs(entity, serverSequence);
    
    // Call the reconciliation callback if set
    if (reconciliationCallback_) {
        reconciliationCallback_(entityId, serverPosition, oldPosition);
    }
    
    return true;
}

void ReconciliationSystem::reapplyInputs(
    std::shared_ptr<NetworkedEntity> entity,
    uint32_t serverSequence) {
    
    uint32_t entityId = entity->getId();
    
    // Get all inputs that came after the server's acknowledged sequence
    auto pendingInputs = predictionSystem_.getSnapshotManager()
                        .getInputSnapshotsAfter(entityId, serverSequence);
    
    if (pendingInputs.empty()) {
        LOG_DEBUG("No inputs to reapply for entity " + std::to_string(entityId), "ReconciliationSystem");
        return;
    }
    
    LOG_DEBUG("Reapplying " + std::to_string(pendingInputs.size()) + 
             " inputs for entity " + std::to_string(entityId), "ReconciliationSystem");
    
    // Reapply each input in sequence
    for (const auto& input : pendingInputs) {
        entity->move(input.movement);
        if (input.isJumping) {
            entity->jump();
        }
        entity->update();
        
        // Update snapshot with new predicted position
        EntitySnapshot newSnapshot;
        newSnapshot.entityId = entityId;
        newSnapshot.position = entity->getPosition();
        newSnapshot.velocity = {0, 0, 0}; // Velocity not tracked in base interface
        newSnapshot.isJumping = input.isJumping;
        newSnapshot.timestamp = std::chrono::steady_clock::now();
        newSnapshot.sequenceNumber = input.sequenceNumber;
        predictionSystem_.getSnapshotManager().storeEntitySnapshot(newSnapshot);
    }
}

void ReconciliationSystem::setReconciliationThreshold(float threshold) {
    reconciliationThreshold_ = threshold;
    LOG_INFO("Set reconciliation threshold to " + std::to_string(threshold), "ReconciliationSystem");
}

float ReconciliationSystem::getReconciliationThreshold() const {
    return reconciliationThreshold_;
}

void ReconciliationSystem::setReconciliationCallback(
    std::function<void(uint32_t, const netcode::math::MyVec3&, const netcode::math::MyVec3&)> callback) {
    reconciliationCallback_ = callback;
}

} // namespace netcode 