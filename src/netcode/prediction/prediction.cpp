#include "netcode/prediction/prediction.hpp"
#include "netcode/utils/logger.hpp"

namespace netcode {

PredictionSystem::PredictionSystem(SnapshotManager& snapshotManager)
    : snapshotManager_(snapshotManager), currentSequence_(0) {
    LOG_INFO("Prediction system initialized", "PredictionSystem");
}

uint32_t PredictionSystem::applyInputPrediction(
    std::shared_ptr<NetworkedEntity> entity,
    const netcode::math::MyVec3& input,
    bool isJumping) {
    
    if (!entity) {
        LOG_ERROR("Null entity passed to prediction system", "PredictionSystem");
        return currentSequence_;
    }
    
    // Store reference to the entity
    entities_[entity->getId()] = entity;
    
    // Apply movement locally
    entity->move(input);
    if (isJumping) {
        entity->jump();
    }
    entity->update();
    
    // Increment sequence number
    uint32_t sequence = getNextSequenceNumber();
    
    // Store input snapshot
    InputSnapshot inputSnapshot;
    inputSnapshot.playerId = entity->getId();
    inputSnapshot.movement = input;
    inputSnapshot.isJumping = isJumping;
    inputSnapshot.timestamp = std::chrono::steady_clock::now();
    inputSnapshot.sequenceNumber = sequence;
    snapshotManager_.storeInputSnapshot(inputSnapshot);
    
    // Store entity snapshot
    EntitySnapshot snapshot;
    snapshot.entityId = entity->getId();
    snapshot.position = entity->getPosition();
    snapshot.velocity = {0, 0, 0}; // Velocity not tracked in base interface
    snapshot.isJumping = isJumping;
    snapshot.timestamp = std::chrono::steady_clock::now();
    snapshot.sequenceNumber = sequence;
    snapshotManager_.storeEntitySnapshot(snapshot);
    
    LOG_DEBUG("Applied prediction for entity " + std::to_string(entity->getId()) + 
              " with sequence " + std::to_string(sequence), "PredictionSystem");
    
    return sequence;
}

uint32_t PredictionSystem::getNextSequenceNumber() {
    return ++currentSequence_;
}

uint32_t PredictionSystem::getCurrentSequenceNumber() const {
    return currentSequence_;
}

void PredictionSystem::reset() {
    currentSequence_ = 0;
    entities_.clear();
    LOG_INFO("Prediction system reset", "PredictionSystem");
}

SnapshotManager& PredictionSystem::getSnapshotManager() {
    return snapshotManager_;
}

} // namespace netcode 