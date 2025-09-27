#include "controllers/WiFiController.h"
#include "services/WiFiManagerService.h"
#include <json/json.h>

void WiFiController::scanNetworks(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto& wifiService = WiFiManagerService::getInstance();
    auto networks = wifiService.scanNetworks();
    
    Json::Value jsonResponse;
    jsonResponse["status"] = "success";
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
