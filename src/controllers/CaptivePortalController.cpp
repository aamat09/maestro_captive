#include "controllers/CaptivePortalController.h"
#include <fstream>
#include <sstream>

void CaptivePortalController::index(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::ifstream file("/opt/maestro/captive/web/templates/index.html");
    if (!file.is_open()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Template not found");
        callback(resp);
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(buffer.str());
    callback(resp);
}

void CaptivePortalController::captiveDetect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    // Handle captive portal detection requests from various devices
    auto resp = HttpResponse::newRedirectionResponse("http://192.168.4.1/");
    callback(resp);
}
