#include "netcode/utils/logger.hpp"

namespace netcode::utils {

    Logger::Logger() : current_level_(LogLevel::INFO) {}

    Logger::~Logger() {
        close_log_file();
    }

    Logger& Logger::get_instance() {
        static Logger instance;
        return instance;
    }

    void Logger::set_level(LogLevel level) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        current_level_ = level;
    }

    bool Logger::set_log_file(const std::string &filename) {
        std::lock_guard<std::mutex> lock(log_mutex_);

        if (log_file_.is_open()) {
            log_file_.close();
        }

        log_file_.open(filename, std::ios::out | std::ios::app);

        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return false;
        }
        return true;
    }

    void Logger::close_log_file() {
        std::lock_guard<std::mutex> lock(log_mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    void Logger::register_callback(std::function<void(LogLevel, const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        callbacks_.push_back(callback);
    }

    void Logger::debug(const std::string& message, const std::string& component) {
        log(LogLevel::DEBUG, message, component);
    }

    void Logger::info(const std::string& message, const std::string& component) {
        log(LogLevel::INFO, message, component);
    }

    void Logger::warning(const std::string& message, const std::string& component) {
        log(LogLevel::WARNING, message, component);
    }

    void Logger::error(const std::string& message, const std::string& component) {
        log(LogLevel::ERROR, message, component);
    }

    void Logger::log(LogLevel level, const std::string& message, const std::string& component) {
        if (level < current_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(log_mutex_);

        std::string timestamp = get_current_time();
        std::string level_str = level_to_string(level);
        std::string formatted_message = "[" + timestamp + "] [" + level_str + "] [" + component + "] " + message;

        std::cout << formatted_message << std::endl;

        if (log_file_.is_open()) {
            log_file_ << formatted_message << std::endl;
            log_file_.flush();
        }

        for (const auto& callback : callbacks_) {
            callback(level, formatted_message);
        }
    }

    std::string Logger::get_current_time() {
        const auto now = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()).count() % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms;


        return ss.str();
    }

    std::string Logger::level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::NONE:    return "NONE";
            default:                return "UNKNOWN";
        }
    }

}
