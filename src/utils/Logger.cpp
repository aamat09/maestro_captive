#include "utils/Logger.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(logMutex);
    baseLogPath = filepath;
    openDailyLogFile();
}

void Logger::openDailyLogFile() {
    // Close current file if open
    if (logFile.is_open()) {
        logFile.close();
    }

    // Generate filename with today's date
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream dateStream;
    dateStream << std::put_time(&tm, "%Y-%m-%d");
    std::string dateStr = dateStream.str();

    std::string dailyLogPath = baseLogPath + "." + dateStr;

    // Open log file for today
    logFile.open(dailyLogPath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << dailyLogPath << std::endl;
    }

    currentLogDate = dateStr;

    // Clean up old log files (keep only 365 days)
    cleanupOldLogs();
}

void Logger::cleanupOldLogs() {
    if (baseLogPath.empty()) return;

    try {
        namespace fs = std::filesystem;
        fs::path logDir = fs::path(baseLogPath).parent_path();
        std::string logBaseName = fs::path(baseLogPath).filename().string();

        std::vector<fs::path> logFiles;

        // Find all log files matching the pattern
        for (const auto& entry : fs::directory_iterator(logDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(logBaseName + ".") == 0) {
                    logFiles.push_back(entry.path());
                }
            }
        }

        // Sort by modification time (oldest first)
        std::sort(logFiles.begin(), logFiles.end(), [](const fs::path& a, const fs::path& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        });

        // Remove files if we have more than 365
        if (logFiles.size() > 365) {
            size_t toRemove = logFiles.size() - 365;
            for (size_t i = 0; i < toRemove; ++i) {
                fs::remove(logFiles[i]);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error cleaning up old logs: " << e.what() << std::endl;
    }
}

void Logger::setLevel(Level level) {
    minLevel = level;
}

void Logger::debug(const std::string& message) {
    log(DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(INFO, message);
}

void Logger::warning(const std::string& message) {
    log(WARNING, message);
}

void Logger::error(const std::string& message) {
    log(ERROR, message);
}

void Logger::log(Level level, const std::string& message) {
    if (level < minLevel) return;

    std::lock_guard<std::mutex> lock(logMutex);

    // Check if we need to rotate to a new day's log file
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream dateStream;
    dateStream << std::put_time(&tm, "%Y-%m-%d");
    std::string dateStr = dateStream.str();

    if (dateStr != currentLogDate && !baseLogPath.empty()) {
        openDailyLogFile();
    }

    std::string timestamp = getCurrentTime();
    std::string levelStr = getLevelString(level);
    std::string logLine = "[" + timestamp + "] [" + levelStr + "] " + message;

    // Write to file
    if (logFile.is_open()) {
        logFile << logLine << std::endl;
        logFile.flush();
    }

    // Also write to stderr for systemd journal
    std::cerr << logLine << std::endl;
}

std::string Logger::getLevelString(Level level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
