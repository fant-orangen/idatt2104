#include "netcode/prediction/snapshot.hpp"
#include "netcode/utils/logger.hpp"
#include <algorithm>

namespace netcode {

void SnapshotManager::storeEntitySnapshot(const EntitySnapshot& snapshot) {
    entitySnapshots_[snapshot.entityId].push_back(snapshot);
    
    // Sort snapshots by sequence number to ensure they're in order
    std::sort(entitySnapshots_[snapshot.entityId].begin(), 
              entitySnapshots_[snapshot.entityId].end());
}

void SnapshotManager::storeInputSnapshot(const InputSnapshot& input) {
    inputSnapshots_[input.playerId].push_back(input);
    
    // Sort inputs by sequence number to ensure they're in order
    std::sort(inputSnapshots_[input.playerId].begin(), 
              inputSnapshots_[input.playerId].end());
}

EntitySnapshot SnapshotManager::getLatestEntitySnapshot(uint32_t entityId) const {
    auto it = entitySnapshots_.find(entityId);
    if (it != entitySnapshots_.end() && !it->second.empty()) {
        return it->second.back(); // Return the most recent snapshot
    }
    
    // Return an empty snapshot if none exists
    EntitySnapshot emptySnapshot;
    emptySnapshot.entityId = entityId;
    emptySnapshot.sequenceNumber = 0;
    return emptySnapshot;
}

std::vector<EntitySnapshot> SnapshotManager::getEntitySnapshotsAfter(
    uint32_t entityId, uint32_t afterSequence) const {
    
    std::vector<EntitySnapshot> result;
    auto it = entitySnapshots_.find(entityId);
    
    if (it != entitySnapshots_.end()) {
        // Find the first snapshot with sequence number > afterSequence
        for (const auto& snapshot : it->second) {
            if (snapshot.sequenceNumber > afterSequence) {
                result.push_back(snapshot);
            }
        }
    }
    
    return result;
}

std::vector<InputSnapshot> SnapshotManager::getInputSnapshotsAfter(
    uint32_t playerId, uint32_t afterSequence) const {
    
    std::vector<InputSnapshot> result;
    auto it = inputSnapshots_.find(playerId);
    
    if (it != inputSnapshots_.end()) {
        // Find all inputs with sequence number > afterSequence
        for (const auto& input : it->second) {
            if (input.sequenceNumber > afterSequence) {
                result.push_back(input);
            }
        }
    }
    
    return result;
}

void SnapshotManager::pruneOldSnapshots(uint64_t maxAge) {
    auto now = std::chrono::steady_clock::now();
    
    // Prune entity snapshots
    for (auto& [entityId, snapshots] : entitySnapshots_) {
        auto newEnd = std::remove_if(snapshots.begin(), snapshots.end(),
            [now, maxAge](const EntitySnapshot& snapshot) {
                auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - snapshot.timestamp).count();
                return age > maxAge;
            });
        
        if (newEnd != snapshots.end()) {
            snapshots.erase(newEnd, snapshots.end());
            LOG_DEBUG("Pruned " + std::to_string(std::distance(newEnd, snapshots.end())) + 
                     " old snapshots for entity " + std::to_string(entityId), "SnapshotManager");
        }
    }
    
    // Prune input snapshots
    for (auto& [playerId, inputs] : inputSnapshots_) {
        auto newEnd = std::remove_if(inputs.begin(), inputs.end(),
            [now, maxAge](const InputSnapshot& input) {
                auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - input.timestamp).count();
                return age > maxAge;
            });
        
        if (newEnd != inputs.end()) {
            inputs.erase(newEnd, inputs.end());
            LOG_DEBUG("Pruned " + std::to_string(std::distance(newEnd, inputs.end())) + 
                     " old input snapshots for player " + std::to_string(playerId), "SnapshotManager");
        }
    }
}

std::shared_ptr<NetworkedEntity> SnapshotManager::getEntity(uint32_t entityId) const {
    auto it = entities_.find(entityId);
    if (it != entities_.end()) {
        // Try to lock the weak_ptr to get a shared_ptr
        auto entity = it->second.lock();
        if (entity) {
            return entity;
        } else {
            // Remove expired reference
            entities_.erase(it);
        }
    }
    return nullptr;
}

void SnapshotManager::registerEntity(uint32_t entityId, std::shared_ptr<NetworkedEntity> entity) {
    if (entity) {
        entities_[entityId] = entity;
        LOG_DEBUG("Registered entity with ID " + std::to_string(entityId), "SnapshotManager");
    } else {
        LOG_WARNING("Attempted to register null entity for ID " + std::to_string(entityId), "SnapshotManager");
    }
}

} // namespace netcode 