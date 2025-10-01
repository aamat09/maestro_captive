#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class WiFiController : public HttpController<WiFiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WiFiController::scanNetworks, "/api/wifi/scan", Get);
    ADD_METHOD_TO(WiFiController::connectToNetwork, "/api/wifi/connect", Post);
    ADD_METHOD_TO(WiFiController::getStatus, "/api/wifi/status", Get);
    ADD_METHOD_TO(WiFiController::disconnect, "/api/wifi/disconnect", Post);
    ADD_METHOD_TO(WiFiController::validateConnectivity, "/api/wifi/validate", Get);
    ADD_METHOD_TO(WiFiController::resetNetwork, "/api/wifi/reset", Post);
    METHOD_LIST_END

    void scanNetworks(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void connectToNetwork(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void getStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void disconnect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void validateConnectivity(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void resetNetwork(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
