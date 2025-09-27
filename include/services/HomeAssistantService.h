#pragma once

class HomeAssistantService {
public:
    static HomeAssistantService& getInstance();
    bool initialize();
    bool start();
    bool stop();
    bool isRunning();
    
private:
    HomeAssistantService() = default;
    bool initialized = false;
};
