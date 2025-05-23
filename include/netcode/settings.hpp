#pragma once

namespace netcode {

/**
 * @brief Abstract interface for network and game settings
 * This interface allows different components to access settings without 
 * being tightly coupled to specific implementations
 */
class ISettings {
public:
    virtual ~ISettings() = default;
    
    // Network Delay Settings (in milliseconds)
    virtual int getClientToServerDelay() const = 0;
    virtual int getServerToClientDelay() const = 0;
    
    // Reconciliation Settings
    virtual bool isPredictionEnabled() const = 0;
    virtual bool isInterpolationEnabled() const = 0;
};

} // namespace netcode 