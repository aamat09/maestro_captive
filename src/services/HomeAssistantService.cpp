#include "services/HomeAssistantService.h"
#include <cstdlib>

HomeAssistantService& HomeAssistantService::getInstance() {
    static HomeAssistantService instance;
    return instance;
}

bool HomeAssistantService::initialize() {
    initialized = true;
    return true;
}

bool HomeAssistantService::start() {
    int result = system("cd /opt/maestro && docker-compose up -d homeassistant");
    return result == 0;
}

bool HomeAssistantService::stop() {
    int result = system("cd /opt/maestro && docker-compose down");
    return result == 0;
}

bool HomeAssistantService::isRunning() {
    int result = system("cd /opt/maestro && docker-compose ps homeassistant | grep -q Up");
    return result == 0;
}
