#include "services/HomeAssistantService.h"
#include "utils/ConfigManager.h"
#include <cstdlib>
#include <string>
#include <sstream>

HomeAssistantService& HomeAssistantService::getInstance() {
    static HomeAssistantService instance;
    return instance;
}

bool HomeAssistantService::initialize() {
    initialized = true;
    return true;
}

bool HomeAssistantService::start() {
    auto& config = ConfigManager::getInstance();
    std::string dockerPath = config.get("DOCKER_COMPOSE_PATH", "/opt/maestro");
    std::string serviceName = config.get("HOME_ASSISTANT_SERVICE_NAME", "homeassistant");

    std::stringstream cmd;
    cmd << "cd " << dockerPath << " && docker-compose up -d " << serviceName;

    int result = system(cmd.str().c_str());
    return result == 0;
}

bool HomeAssistantService::stop() {
    auto& config = ConfigManager::getInstance();
    std::string dockerPath = config.get("DOCKER_COMPOSE_PATH", "/opt/maestro");
    std::string serviceName = config.get("HOME_ASSISTANT_SERVICE_NAME", "homeassistant");

    std::stringstream cmd;
    cmd << "cd " << dockerPath << " && docker-compose stop " << serviceName;

    int result = system(cmd.str().c_str());
    return result == 0;
}

bool HomeAssistantService::isRunning() {
    auto& config = ConfigManager::getInstance();
    std::string dockerPath = config.get("DOCKER_COMPOSE_PATH", "/opt/maestro");
    std::string serviceName = config.get("HOME_ASSISTANT_SERVICE_NAME", "homeassistant");

    std::stringstream cmd;
    cmd << "cd " << dockerPath << " && docker-compose ps " << serviceName << " | grep -q Up";

    int result = system(cmd.str().c_str());
    return result == 0;
}
