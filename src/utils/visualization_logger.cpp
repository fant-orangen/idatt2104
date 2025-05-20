#include "netcode/utils/visualization_logger.hpp"

namespace netcode::utils {

    visualization::GameWindow* VisualizationLogger::window_ = nullptr;

    void VisualizationLogger::initialize(visualization::GameWindow *window) {
        window_ = window;

        Logger::get_instance().register_callback(log_callback);
    }

    void VisualizationLogger::shutdown() {
        window_ = nullptr;
    }

    void VisualizationLogger::log_callback(LogLevel level, const std::string& message) {
        if (window_ != nullptr) {
            if (level >= LogLevel::INFO) {
                window_->add_network_message(message);
            }
        }
    }

}