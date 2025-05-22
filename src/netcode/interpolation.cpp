#include "netcode/interpolation.hpp"
#include "netcode/utils/logger.hpp"
#include <algorithm>

namespace netcode {

InterpolationSystem::InterpolationSystem(SnapshotManager& snapshotManager, const InterpolationConfig& config)
    : snapshotManager_(snapshotManager), config_(config) {
    LOG_INFO("Interpolation system initialized with delay " + 
             std::to_string(config.interpolationDelay) + "ms", "InterpolationSystem");
}

void InterpolationSystem::updateEntity(std::shared_ptr<NetworkedEntity> entity, float deltaTime) {
    if (!entity) {
        LOG_ERROR("Null entity passed to interpolation system", "InterpolationSystem");
        return;
    }
    
    uint32_t entityId = entity->getId();
    
    // Initialize render time for this entity if it doesn't exist
    if (renderTimes_.find(entityId) == renderTimes_.end()) {
        renderTimes_[entityId] = std::chrono::steady_clock::now() - 
            std::chrono::milliseconds(config_.interpolationDelay);
    }
    
    // Advance render time
    renderTimes_[entityId] += std::chrono::microseconds(static_cast<int64_t>(deltaTime * 1000000));
    
    // Find appropriate snapshots for interpolation
    EntitySnapshot startSnapshot;
    EntitySnapshot endSnapshot;
    float t = 0.0f;
    
    bool haveSnapshots = findInterpolationSnapshots(entityId, renderTimes_[entityId], 
                                                   startSnapshot, endSnapshot, t);
    
    if (!haveSnapshots) {
        // No suitable snapshots found, can't interpolate
        LOG_DEBUG("No suitable snapshots found for entity " + std::to_string(entityId), "InterpolationSystem");
        return;
    }
    
    // Calculate interpolated position
    netcode::math::MyVec3 currentPos = entity->getPosition();
    netcode::math::MyVec3 targetPos = Lerp(startSnapshot.position, endSnapshot.position, t);
    
    // Check if we need to snap instead of interpolate
    float distance = Magnitude(targetPos - currentPos);
    if (distance > config_.maxInterpolationDistance) {
        LOG_INFO("Snapping entity " + std::to_string(entityId) + 
                " due to large distance: " + std::to_string(distance), "InterpolationSystem");
        entity->setPosition(targetPos);
    } else {
        // Smooth interpolation
        entity->setPosition(targetPos);
    }
    
    // Handle jumping state if needed
    if (endSnapshot.isJumping && !startSnapshot.isJumping) {
        entity->jump();
    }
    
    entity->update();
}

void InterpolationSystem::recordEntityPosition(
    uint32_t entityId,
    const netcode::math::MyVec3& position,
    std::chrono::steady_clock::time_point timestamp) {
    
    // Create and store a snapshot with an incremented sequence number
    EntitySnapshot snapshot;
    snapshot.entityId = entityId;
    snapshot.position = position;
    snapshot.velocity = {0, 0, 0}; // Velocity not tracked
    snapshot.isJumping = false; // We don't know this from the position update
    snapshot.timestamp = timestamp;
    
    // Use the latest snapshot's sequence + 1, or 0 if none exists
    EntitySnapshot latestSnapshot = snapshotManager_.getLatestEntitySnapshot(entityId);
    snapshot.sequenceNumber = latestSnapshot.sequenceNumber + 1;
    
    snapshotManager_.storeEntitySnapshot(snapshot);
    
    LOG_DEBUG("Recorded position for entity " + std::to_string(entityId) +
             " at sequence " + std::to_string(snapshot.sequenceNumber), "InterpolationSystem");
}

bool InterpolationSystem::findInterpolationSnapshots(
    uint32_t entityId,
    std::chrono::steady_clock::time_point renderTime,
    EntitySnapshot& out_start,
    EntitySnapshot& out_end,
    float& out_t) {
    
    // Get all snapshots for this entity
    auto latestSnapshot = snapshotManager_.getLatestEntitySnapshot(entityId);
    auto allSnapshots = snapshotManager_.getEntitySnapshotsAfter(entityId, 0); // Get all snapshots
    
    if (allSnapshots.empty()) {
        return false;
    }
    
    // Sort snapshots by timestamp
    std::sort(allSnapshots.begin(), allSnapshots.end(), 
             [](const EntitySnapshot& a, const EntitySnapshot& b) {
                 return a.timestamp < b.timestamp;
             });
    
    // Find the first snapshot with timestamp >= renderTime
    auto it = std::find_if(allSnapshots.begin(), allSnapshots.end(),
                          [renderTime](const EntitySnapshot& snapshot) {
                              return snapshot.timestamp >= renderTime;
                          });
    
    if (it == allSnapshots.begin()) {
        // All snapshots are newer than renderTime, use the oldest
        out_end = *it;
        out_start = *it;
        out_t = 0.0f;
        return true;
    } else if (it == allSnapshots.end()) {
        // All snapshots are older than renderTime, use the newest
        out_start = allSnapshots.back();
        out_end = allSnapshots.back();
        out_t = 1.0f;
        return true;
    } else {
        // We have snapshots on both sides, interpolate between them
        out_end = *it;
        out_start = *(it - 1);
        
        // Calculate t as a value between 0 and 1
        auto startToEnd = std::chrono::duration_cast<std::chrono::microseconds>(
            out_end.timestamp - out_start.timestamp).count();
        auto startToRender = std::chrono::duration_cast<std::chrono::microseconds>(
            renderTime - out_start.timestamp).count();
        
        // Avoid division by zero
        if (startToEnd > 0) {
            out_t = static_cast<float>(startToRender) / static_cast<float>(startToEnd);
            out_t = std::max(0.0f, std::min(1.0f, out_t)); // Clamp between 0 and 1
        } else {
            out_t = 0.0f;
        }
        
        return true;
    }
}

void InterpolationSystem::setConfig(const InterpolationConfig& config) {
    config_ = config;
    LOG_INFO("Updated interpolation config with delay " + 
             std::to_string(config.interpolationDelay) + "ms", "InterpolationSystem");
}

const InterpolationConfig& InterpolationSystem::getConfig() const {
    return config_;
}

void InterpolationSystem::reset() {
    renderTimes_.clear();
    interpolationTargets_.clear();
    LOG_INFO("Interpolation system reset", "InterpolationSystem");
}

} // namespace netcode 