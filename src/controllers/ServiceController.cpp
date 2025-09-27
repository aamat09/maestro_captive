#include "controllers/ServiceController.h"
#include "services/HomeAssistantService.h"
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
        // Stop the hotspot connection
        int hotspotResult = system("systemctl stop maestro-hotspot.service");
        
        // Stop the DHCP/DNS service
        int dhcpResult = system("systemctl stop maestro-dhcp.service");
        
        // Disable services to prevent auto-restart
        system("systemctl disable maestro-hotspot.service");
        system("systemctl disable maestro-dhcp.service");
        
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
