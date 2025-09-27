#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ServiceController : public HttpController<ServiceController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ServiceController::startHomeAssistant, "/api/services/homeassistant/start", Post);
    ADD_METHOD_TO(ServiceController::stopHomeAssistant, "/api/services/homeassistant/stop", Post);
    ADD_METHOD_TO(ServiceController::getServiceStatus, "/api/services/status", Get);
    ADD_METHOD_TO(ServiceController::shutdownHotspot, "/api/services/hotspot/shutdown", Post);
    METHOD_LIST_END
    
    void startHomeAssistant(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void stopHomeAssistant(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void getServiceStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void shutdownHotspot(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
