#include "controllers/ServiceController.h"
#include "services/HomeAssistantService.h"
#include "utils/ConfigManager.h"
#include <json/json.h>
#include <cstdlib>

void ServiceController::startHomeAssistant(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& haService = HomeAssistantService::getInstance();
    bool success = haService.start();
    
    Json::Value response;
    response["status"] = success ? "success" : "error";
    response["message"] = success ? "Home Assistant started" : "Failed to start Home Assistant";
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ServiceController::stopHomeAssistant(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& haService = HomeAssistantService::getInstance();
    bool success = haService.stop();
    
    Json::Value response;
    response["status"] = success ? "success" : "error";
    response["message"] = success ? "Home Assistant stopped" : "Failed to stop Home Assistant";
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ServiceController::getServiceStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& haService = HomeAssistantService::getInstance();
    
    Json::Value response;
    response["status"] = "success";
    response["homeassistant"] = haService.isRunning();
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ServiceController::shutdownHotspot(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    Json::Value response;

    try {
        auto& config = ConfigManager::getInstance();
        std::string hotspotService = config.get("HOTSPOT_SERVICE_NAME", "maestro-hotspot.service");
        std::string dhcpService = config.get("DHCP_SERVICE_NAME", "maestro-dhcp.service");

        // Stop the hotspot connection
        std::string stopHotspotCmd = "systemctl stop " + hotspotService;
        int hotspotResult = system(stopHotspotCmd.c_str());

        // Stop the DHCP/DNS service
        std::string stopDhcpCmd = "systemctl stop " + dhcpService;
        int dhcpResult = system(stopDhcpCmd.c_str());

        // Disable services to prevent auto-restart
        std::string disableHotspotCmd = "systemctl disable " + hotspotService;
        std::string disableDhcpCmd = "systemctl disable " + dhcpService;
        system(disableHotspotCmd.c_str());
        system(disableDhcpCmd.c_str());

        if (hotspotResult == 0 && dhcpResult == 0) {
            response["status"] = "success";
            response["message"] = "Hotspot and DHCP services shutdown successfully";
        } else {
            response["status"] = "warning";
            response["message"] = "Some services may not have stopped cleanly";
        }

    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = std::string("Failed to shutdown services: ") + e.what();
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}
