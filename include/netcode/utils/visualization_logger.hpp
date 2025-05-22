#pragma once
#include "netcode/utils/logger.hpp"
#include "netcode/visualization/game_window.hpp"
#include <memory>

namespace netcode::utils {

    /**
     * @brief A logging utility class that integrates with the game visualization window.
     * 
     * The VisualizationLogger class provides functionality to log messages directly to
     * the game window interface. It acts as a bridge between the logging system and
     * the visual display, allowing real-time visualization of log messages in the
     * game window.
     */
    class VisualizationLogger {

    public:
        /**
         * @brief Initializes the visualization logger with a game window instance.
         * 
         * @param window Pointer to the game window where log messages will be displayed.
         *              The window instance must remain valid throughout the logger's lifetime.
         */
        static void initialize(visualization::GameWindow* window);

        /**
         * @brief Shuts down the visualization logger and cleans up resources.
         * 
         * This method should be called before the game window is destroyed to ensure
         * proper cleanup of logging resources.
         */
        static void shutdown();

    private:
        /**
         * @brief Callback function that handles the actual logging of messages.
         * 
         * @param level The severity level of the log message.
         * @param message The content of the log message to be displayed.
         */
        static void log_callback(LogLevel level, const std::string& message);

        /**
         * @brief Pointer to the game window instance used for displaying log messages.
         */
        static visualization::GameWindow* window_;
    };

}
