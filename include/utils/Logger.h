#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static Logger& getInstance();
    void setLogFile(const std::string& filepath);
    void setLevel(Level level);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    Logger() : minLevel(INFO) {}
    void log(Level level, const std::string& message);
    void openDailyLogFile();
    void cleanupOldLogs();
    std::string getLevelString(Level level);
    std::string getCurrentTime();

    std::ofstream logFile;
    std::mutex logMutex;
    Level minLevel;
    std::string baseLogPath;
    std::string currentLogDate;
};
