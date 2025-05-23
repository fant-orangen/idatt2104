#pragma once

/**
 * @file netcode.hpp
 * @brief Main header for the netcode functionality
 * 
 * This file includes all the necessary netcode components for
 * client-side prediction, server reconciliation, and entity interpolation.
 */

#include "snapshot.hpp"
#include "prediction.hpp"
#include "reconciliation.hpp"
#include "interpolation.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/math/my_vec3.hpp"

namespace netcode {

/**
 * @brief Initialize a complete netcode system with all components
 * 
 * Helper function to create and connect all netcode components
 * 
 * @param interpolationDelay Milliseconds of delay for interpolation
 * @param reconciliationThreshold Distance threshold for triggering reconciliation
 * @return A tuple containing all system components
 */
inline std::tuple<
    SnapshotManager,
    PredictionSystem,
    ReconciliationSystem,
    InterpolationSystem
> createNetcodeSystem(
    uint32_t interpolationDelay = 100,
    float reconciliationThreshold = 0.5f
) {
    // Create components
    SnapshotManager snapshotManager;
    PredictionSystem predictionSystem(snapshotManager);
    ReconciliationSystem reconciliationSystem(predictionSystem);
    
    // Configure interpolation
    InterpolationConfig interpolationConfig;
    interpolationConfig.interpolationDelay = interpolationDelay;
    InterpolationSystem interpolationSystem(snapshotManager, interpolationConfig);
    
    // Configure reconciliation
    reconciliationSystem.setReconciliationThreshold(reconciliationThreshold);
    
    return {
        std::move(snapshotManager),
        std::move(predictionSystem),
        std::move(reconciliationSystem),
        std::move(interpolationSystem)
    };
}

} // namespace netcode 