#include "controllers/WiFiController.h"
#include "services/WiFiManagerService.h"
#include "utils/ConfigManager.h"
#include "utils/Logger.h"
#include <json/json.h>

void WiFiController::scanNetworks(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    // Check for full_scan parameter
    bool full_scan = false;
    auto params = req->getParameters();
    if (params.find("full_scan") != params.end()) {
        full_scan = (params.at("full_scan") == "true" || params.at("full_scan") == "1");
    }

    auto& wifiService = WiFiManagerService::getInstance();
    auto networks = wifiService.scanNetworks(full_scan);

    Json::Value jsonResponse;
    jsonResponse["status"] = "success";
    jsonResponse["scan_type"] = full_scan ? "full" : "cached";
    jsonResponse["networks"] = Json::Value(Json::arrayValue);

    for (const auto& network : networks) {
        Json::Value networkJson;
        networkJson["ssid"] = network.ssid;
        networkJson["signal"] = network.signal;
        networkJson["security"] = network.security;
        jsonResponse["networks"].append(networkJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    callback(resp);
}

void WiFiController::connectToNetwork(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        Json::Value error;
        error["status"] = "error";
        error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string ssid = (*jsonBody)["ssid"].asString();
    std::string password = (*jsonBody)["password"].asString();
    
    auto& wifiService = WiFiManagerService::getInstance();
    bool success = wifiService.connectToNetwork(ssid, password);
    
    Json::Value response;
    response["status"] = success ? "success" : "error";
    if (!success) {
        response["message"] = "Failed to connect to network";
    }
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    if (!success) {
        resp->setStatusCode(k500InternalServerError);
    }
    callback(resp);
}

void WiFiController::getStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& wifiService = WiFiManagerService::getInstance();
    auto status = wifiService.getConnectionStatus();
    
    Json::Value response;
    response["status"] = "success";
    response["connected"] = status.connected;
    response["ssid"] = status.ssid;
    response["signal"] = status.signal;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void WiFiController::disconnect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& wifiService = WiFiManagerService::getInstance();
    bool success = wifiService.disconnect();
    
    Json::Value response;
    response["status"] = success ? "success" : "error";
    if (!success) {
        response["message"] = "Failed to disconnect";
    }
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void WiFiController::validateConnectivity(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& wifiService = WiFiManagerService::getInstance();

    Json::Value response;
    response["status"] = "success";

    // Check if we're connected to WiFi
    auto status = wifiService.getConnectionStatus();
    response["wifi_connected"] = status.connected;
    response["ssid"] = status.ssid;

    if (status.connected) {
        // Validate internet connectivity
        bool hasInternet = wifiService.validateInternetConnectivity();
        response["internet_connected"] = hasInternet;
        response["message"] = hasInternet ? "Full connectivity validated" : "WiFi connected but no internet access";
    } else {
        response["internet_connected"] = false;
        response["message"] = "Not connected to WiFi";
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void WiFiController::resetNetwork(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& wifiService = WiFiManagerService::getInstance();
    auto& config = ConfigManager::getInstance();
    auto& logger = Logger::getInstance();

    Json::Value response;

    try {
        logger.info("Network reset requested");

        // Disconnect from WiFi
        logger.info("Attempting to disconnect from WiFi");
        bool disconnected = wifiService.disconnect();

        if (disconnected) {
            logger.info("WiFi disconnected successfully");

            // Start hotspot services
            std::string hotspotService = config.get("HOTSPOT_SERVICE_NAME", "maestro-hotspot.service");
            std::string dhcpService = config.get("DHCP_SERVICE_NAME", "maestro-dhcp.service");

            logger.info("Starting hotspot service: " + hotspotService);
            std::string startHotspotCmd = "systemctl start " + hotspotService;
            int hotspotResult = system(startHotspotCmd.c_str());

            logger.info("Starting DHCP service: " + dhcpService);
            std::string startDhcpCmd = "systemctl start " + dhcpService;
            int dhcpResult = system(startDhcpCmd.c_str());

            if (hotspotResult == 0 && dhcpResult == 0) {
                logger.info("Hotspot services started successfully");
                response["status"] = "success";
                response["message"] = "Network reset complete. Hotspot is now active.";
            } else {
                logger.error("Failed to start hotspot services (hotspot=" + std::to_string(hotspotResult) + ", dhcp=" + std::to_string(dhcpResult) + ")");
                response["status"] = "warning";
                response["message"] = "WiFi disconnected but failed to start hotspot";
            }
        } else {
            logger.error("Failed to disconnect from WiFi");
            response["status"] = "error";
            response["message"] = "Failed to disconnect from WiFi";
        }
    } catch (const std::exception& e) {
        logger.error("Network reset exception: " + std::string(e.what()));
        response["status"] = "error";
        response["message"] = std::string("Network reset failed: ") + e.what();
    }

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}
