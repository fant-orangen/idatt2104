#ifndef LOGGER_HPP
#define LOGGER_HPP

#endif //LOGGER_HPP

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

namespace netcode::utils {

    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        NONE
    };

    class Logger {
    public:
        Logger();
        ~Logger();

        static Logger& get_instance();

        void set_level(LogLevel level);

        bool set_log_file(const std::string& filename);

        void close_log_file();

        void register_callback(std::function<void(LogLevel, const std::string&)> callback);

        void debug(const std::string& message, const std::string& component = "General");
        void info(const std::string& message, const std::string& component = "General");
        void warning(const std::string& message, const std::string& component = "General");
        void error(const std::string& message, const std::string& component = "General");

        void log(LogLevel level, const std::string& message, const std::string& component = "General");

    private:
        std::mutex log_mutex_;
        LogLevel current_level_;
        std::ofstream log_file_;
        std::vector<std::function<void(LogLevel, const std::string&)>> callbacks_;

        std::string get_current_time();

        static std::string level_to_string(LogLevel level);
    };

#define LOG_DEBUG(msg, component) netcode::utils::Logger::get_instance().debug(msg, component)
#define LOG_INFO(msg, component) netcode::utils::Logger::get_instance().info(msg, component)
#define LOG_WARNING(msg, component) netcode::utils::Logger::get_instance().warning(msg, component)
#define LOG_ERROR(msg, component) netcode::utils::Logger::get_instance().error(msg, component)

}
