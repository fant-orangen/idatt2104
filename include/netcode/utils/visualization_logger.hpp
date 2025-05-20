#pragma once
#include "netcode/utils/logger.hpp"
#include "netcode/visualization/game_window.hpp"
#include <memory>

namespace netcode::utils {

    class VisualizationLogger {

    public:
        static void initialize(visualization::GameWindow* window);
        static void shutdown();

    private:
        static void log_callback(LogLevel level, const std::string& message);
        static visualization::GameWindow* window_;
    };

}
