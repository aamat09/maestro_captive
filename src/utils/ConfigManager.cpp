#include "utils/ConfigManager.h"
#include <fstream>
#include <sstream>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load(const std::string& configFile) {
    configFilePath = configFile;
    std::ifstream file(configFile);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config[key] = value;
        }
    }
    
    return true;
}

std::string ConfigManager::get(const std::string& key, const std::string& defaultValue) {
    auto it = config.find(key);
    return (it != config.end()) ? it->second : defaultValue;
}

void ConfigManager::set(const std::string& key, const std::string& value) {
    config[key] = value;
}

bool ConfigManager::save() {
    if (configFilePath.empty()) return false;
    
    std::ofstream file(configFilePath);
    if (!file.is_open()) return false;
    
    for (const auto& pair : config) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    return true;
}
