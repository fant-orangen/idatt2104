#include "netcode/utils/visualization_logger.hpp"

namespace netcode::utils {

    // Static member initialization
    visualization::GameWindow* VisualizationLogger::window_ = nullptr;

    // Sets up visualization logging by registering the callback with the main logger
    void VisualizationLogger::initialize(visualization::GameWindow *window) {
        window_ = window;

        Logger::get_instance().register_callback(log_callback);
    }

    // Cleans up by nullifying the window pointer
    // Does not unregister the callback as it becomes no-op when window_ is null
    void VisualizationLogger::shutdown() {
        window_ = nullptr;
    }

    // Callback implementation that forwards messages to the game window
    // Only forwards INFO and above messages to avoid cluttering the visualization
    void VisualizationLogger::log_callback(LogLevel level, const std::string& message) {
        if (window_ != nullptr) {
            if (level >= LogLevel::INFO) {
                window_->add_network_message(message);
            }
        }
    }

}