#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>

/**
 *@file logger.hpp
 *@brief Simple logging system for the netcode library
 *
 * Supports different log levels, file output, and custom callbacks.
 */

namespace netcode::utils {

/**
 * @brief Log levels from least to most severe (plus NONE to disable logging)
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    NONE
};

/**
 * @breif Thread-safe logging class that works as a singleton.
 */
class Logger {
public:

    /**
     * @brief Constructor initializing the logger with default values
     */
    Logger();

    /**
     * @brief Destructor ensuring the log file is properly closed
     */
    ~Logger();

    /**
     * @brief Gets the singleton instance of the logger
     * @return Reference to the global Logger instance
     */
    static Logger& get_instance();

    /**
     * @brief Sets the minimum level for log messages to be processed
     * @param level The new minimum level for logging
     */

    void set_level(LogLevel level);

    /**
     * @brief Sets up a file for log output
     * @param filename The path to the file to use for log output
     * @return true if the file was opened successfully, false otherwise
     */
    bool set_log_file(const std::string& filename);

    /**
     * @brief Closes the open log file if one exists
     */
    void close_log_file();

    /**
     * @brief Registers a callback function for custom log handling
     * @param callback The function to be called for each log message
     */

    void register_callback(std::function<void(LogLevel, const std::string&)> callback);

    /**
     * @brief Logs a message at DEBUG level
     * @param message The message to log
     * @param component The name of the component the message belongs to
     */
    void debug(const std::string& message, const std::string& component = "General");

    /**
     * @brief Logs a message at INFO level
     * @param message The message to log
     * @param component The name of the component the message belongs to
     */
    void info(const std::string& message, const std::string& component = "General");

    /**
     * @brief Logs a message at WARNING level
     * @param message The message to log
     * @param component The name of the component the message belongs to
     */
    void warning(const std::string& message, const std::string& component = "General");

    /**
     * @brief Logs a message at ERROR level
     * @param message The message to log
     * @param component The name of the component the message belongs to
     */
    void error(const std::string& message, const std::string& component = "General");

    /**
     * @brief Generic logging method for all log levels
     * @param level The log level for the message
     * @param message The message to log
     * @param component The name of the component the message belongs to
     */
    void log(LogLevel level, const std::string& message, const std::string& component = "General");

private:
    std::mutex log_mutex_;           /**< Mutex for thread safety */
    LogLevel current_level_;         /**< Current minimum log level */
    std::ofstream log_file_;         /**< Output file stream for the log file */
    std::vector<std::function<void(LogLevel, const std::string&)>> callbacks_; /**< Registered callback functions */

    /**
     * @brief Gets the current time formatted as a string
     * @return Current time formatted as "YYYY-MM-DD HH:MM:SS"
     */
    std::string get_current_time();

    /**
     * @brief Converts a LogLevel to its string representation
     * @param level The LogLevel value to convert
     * @return String representation of the given log level
     */
    static std::string level_to_string(LogLevel level);
};

    /**
     * @brief Macro for DEBUG level logging with component name
     * @param msg The message to log
     * @param component The component the message belongs to
     */
#define LOG_DEBUG(msg, component) netcode::utils::Logger::get_instance().debug(msg, component)

    /**
     * @brief Macro for INFO level logging with component name
     * @param msg The message to log
     * @param component The component the message belongs to
     */
#define LOG_INFO(msg, component) netcode::utils::Logger::get_instance().info(msg, component)

    /**
     * @brief Macro for WARNING level logging with component name
     * @param msg The message to log
     * @param component The component the message belongs to
     */
#define LOG_WARNING(msg, component) netcode::utils::Logger::get_instance().warning(msg, component)

    /**
     * @brief Macro for ERROR level logging with component name
     * @param msg The message to log
     * @param component The component the message belongs to
     */
#define LOG_ERROR(msg, component) netcode::utils::Logger::get_instance().error(msg, component)

}