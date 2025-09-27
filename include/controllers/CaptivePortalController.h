#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class CaptivePortalController : public HttpController<CaptivePortalController> {
public:
    METHOD_LIST_BEGIN
    // Main portal pages
    ADD_METHOD_TO(CaptivePortalController::index, "/", Get);
    ADD_METHOD_TO(CaptivePortalController::index, "/index.html", Get);
    
    // Android/Google captive portal detection
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/generate_204", Get);
    
    // Microsoft connectivity tests
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/connecttest.txt", Get);
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/ncsi.txt", Get);
    
    // Apple captive portal detection
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/hotspot-detect.html", Get);
    
    // Firefox captive portal detection
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/success.txt", Get);
    
    // Ubuntu connectivity check
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/connectivity-check", Get);
    
    // Generic redirection catch-all
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/redirect", Get);
    METHOD_LIST_END
    
    void index(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void captiveDetect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
