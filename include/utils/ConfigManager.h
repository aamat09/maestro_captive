#pragma once
#include <string>
#include <map>

class ConfigManager {
public:
    static ConfigManager& getInstance();
    bool load(const std::string& configFile);
    std::string get(const std::string& key, const std::string& defaultValue = "");
    void set(const std::string& key, const std::string& value);
    bool save();
    
private:
    ConfigManager() = default;
    std::map<std::string, std::string> config;
    std::string configFilePath;
};
