#include "netcode/reconciliation.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {

ReconciliationSystem::ReconciliationSystem(PredictionSystem& predictionSystem)
    : predictionSystem_(predictionSystem), reconciliationThreshold_(0.5f), smoothingFactor_(10.0f) {
    LOG_INFO("Reconciliation system initialized", "ReconciliationSystem");
}

bool ReconciliationSystem::reconcileState(
    std::shared_ptr<NetworkedEntity> entity,
    const netcode::math::MyVec3& serverPosition,
    uint32_t serverSequence,
    std::chrono::steady_clock::time_point serverTimestamp,
    bool serverIsJumping) {
    
    if (!entity) {
        LOG_ERROR("Null entity passed to reconciliation system", "ReconciliationSystem");
        return false;
    }
    
    uint32_t entityId = entity->getId();
    
    // Check if enough time has passed since last reconciliation for this entity
    auto now = std::chrono::steady_clock::now();
    auto it = lastReconciliationTimes_.find(entityId);
    
    if (it != lastReconciliationTimes_.end()) {
        auto timeSinceLastReconciliation = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
        if (timeSinceLastReconciliation < MIN_RECONCILIATION_INTERVAL_MS) {
            // Too soon since last reconciliation, skip this one
            LOG_DEBUG("Skipping reconciliation for entity " + std::to_string(entityId) + 
                     " (cooldown: " + std::to_string(timeSinceLastReconciliation) + "ms)", "ReconciliationSystem");
            return false;
        }
    }
    
    netcode::math::MyVec3 clientPosition = entity->getPosition();
    
    // Calculate distance between client and server positions
    float positionDifference = Magnitude(serverPosition - clientPosition);
    
    // If difference is below threshold, no reconciliation needed
    if (positionDifference < reconciliationThreshold_) {
        LOG_DEBUG("No reconciliation needed for entity " + std::to_string(entityId) + 
                 " (diff: " + std::to_string(positionDifference) + ")", "ReconciliationSystem");
        return false;
    }
    
    // Update last reconciliation time
    lastReconciliationTimes_[entityId] = now;
    
    // Log reconciliation event
    LOG_INFO("Reconciling entity " + std::to_string(entityId) + 
             " (diff: " + std::to_string(positionDifference) + ")", "ReconciliationSystem");
    
    // Store positions for callback and for reconciliation state
    netcode::math::MyVec3 oldPosition = clientPosition;
    
    // Instead of setting up a blend that we manage, create a reconciliation state
    // that will be processed on the next update
    ReconciliationState& state = reconciliationStates_[entityId];
    state.startPosition = clientPosition; // Still needed for callback
    state.targetPosition = serverPosition;
    state.reconciling = true;
    state.serverSequence = serverSequence;
    state.serverIsJumping = serverIsJumping;
    
    // Store this server snapshot
    EntitySnapshot serverSnapshot;
    serverSnapshot.entityId = entityId;
    serverSnapshot.position = serverPosition;
    serverSnapshot.velocity = {0, 0, 0}; // Velocity not tracked in base interface
    serverSnapshot.isJumping = serverIsJumping; // Use server's jumping state
    serverSnapshot.timestamp = serverTimestamp;
    serverSnapshot.sequenceNumber = serverSequence;
    predictionSystem_.getSnapshotManager().storeEntitySnapshot(serverSnapshot);
    
    // We'll handle the actual state update in the update() method
    
    // Call the reconciliation callback if set
    if (reconciliationCallback_) {
        reconciliationCallback_(entityId, serverPosition, oldPosition);
    }
    
    return true;
}

void ReconciliationSystem::update(float deltaTime) {
    for (auto it = reconciliationStates_.begin(); it != reconciliationStates_.end(); ) {
        uint32_t entityId = it->first;
        ReconciliationState& state = it->second;
        
        if (!state.reconciling) {
            ++it;
            continue;
        }
        
        auto entityPtr = predictionSystem_.getSnapshotManager().getEntity(entityId);
        if (!entityPtr) {
            it = reconciliationStates_.erase(it);
            continue;
        }
        
        // We're not doing visual blending here anymore - entity handles that
        // Just apply the correct simulation state and trigger the entity's visual blend
        
        // Snap the entity's simulation state to server position and jumping state
        entityPtr->snapSimulationState(state.targetPosition, state.serverIsJumping);
        
        // Reapply inputs to get the final simulation state
        reapplyInputs(entityPtr, state.serverSequence, state.targetPosition);
        
        // Tell the entity to start visual blending
        entityPtr->initiateVisualBlend();
        
        // We're done with this reconciliation
        it = reconciliationStates_.erase(it);
    }
}

void ReconciliationSystem::reapplyInputs(
    std::shared_ptr<NetworkedEntity> entity,
    uint32_t serverSequence,
    const netcode::math::MyVec3& targetPosition) {
    
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
    
    // Set entity to the server position first
    entity->setPosition(targetPosition);
    
    // Reapply each input in sequence
    for (const auto& input : pendingInputs) {
        entity->move(input.movement);
        if (input.isJumping && input.sequenceNumber > serverSequence) {
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

void ReconciliationSystem::setSmoothingFactor(float smoothFactor) {
    smoothingFactor_ = smoothFactor;
    LOG_INFO("Set reconciliation smoothing factor to " + std::to_string(smoothFactor), "ReconciliationSystem");
}

void ReconciliationSystem::reset() {
    reconciliationStates_.clear();
    lastReconciliationTimes_.clear();
    LOG_INFO("Reconciliation system reset", "ReconciliationSystem");
}

} // namespace netcode 