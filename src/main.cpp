#include <drogon/drogon.h>
#include <signal.h>
#include <iostream>
#include "controllers/CaptivePortalController.h"
#include "controllers/WiFiController.h"
#include "controllers/ServiceController.h"
#include "services/WiFiManagerService.h"
#include "services/HomeAssistantService.h"
#include "utils/ConfigManager.h"

using namespace drogon;

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    app().quit();
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "Starting Maestro Captive Portal..." << std::endl;
    
    // Load configuration
    auto& config = ConfigManager::getInstance();
    if (!config.load("/opt/maestro/captive/config/maestro.conf")) {
        std::cerr << "Warning: Could not load configuration file" << std::endl;
    }
    
    // Configure Drogon
    app().setLogPath("/var/log/maestro");
    app().setLogLevel(trantor::Logger::kInfo);
    app().addListener("0.0.0.0", 80);
    app().setDocumentRoot("/opt/maestro/captive/web");
    app().setStaticFilesCacheTime(86400); // 1 day cache for static files
    
    // Set thread pool size
    app().setThreadNum(4);
    
    // Initialize services
    auto& wifiService = WiFiManagerService::getInstance();
    auto& haService = HomeAssistantService::getInstance();
    
    if (!wifiService.initialize()) {
        std::cerr << "Failed to initialize WiFi Manager Service" << std::endl;
        return 1;
    }
    
    if (!haService.initialize()) {
        std::cerr << "Failed to initialize Home Assistant Service" << std::endl;
        return 1;
    }
    
    std::cout << "Maestro Captive Portal started on port 80" << std::endl;
    
    // Run the application
    app().run();
    
    return 0;
}
