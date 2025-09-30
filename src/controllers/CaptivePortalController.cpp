#include "controllers/CaptivePortalController.h"
#include <fstream>
#include <sstream>

void CaptivePortalController::index(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::ifstream file("../web/templates/index.html");
    if (!file.is_open()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Template not found: ../web/templates/index.html");
        callback(resp);
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(buffer.str());
    
    // Add headers to prevent caching
    resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    resp->addHeader("Pragma", "no-cache");
    resp->addHeader("Expires", "0");
    
    callback(resp);
}

void CaptivePortalController::captiveDetect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    // Handle captive portal detection requests from various devices
    std::string userAgent = req->getHeader("User-Agent");
    std::string requestUri = req->getPath();
    
    // Log the detection attempt
    LOG_INFO << "Captive portal detection from: " << userAgent << " requesting: " << requestUri;
    
    // Different devices expect different responses for captive portal detection
    
    // Firefox expects a specific response
    if (requestUri.find("success.txt") != std::string::npos) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        resp->setBody("success");
        callback(resp);
        return;
    }
    
    // Microsoft connectivity test
    if (requestUri.find("ncsi.txt") != std::string::npos || 
        requestUri.find("connecttest.txt") != std::string::npos) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        resp->setBody("Microsoft NCSI");
        callback(resp);
        return;
    }
    
    // Android/Google connectivity check
    if (requestUri.find("generate_204") != std::string::npos) {
        // Instead of 204, redirect to our portal to trigger captive portal detection
        auto resp = HttpResponse::newRedirectionResponse("http://192.168.4.1/");
        resp->setStatusCode(k302Found);
        callback(resp);
        return;
    }
    
    // Apple captive portal detection
    if (requestUri.find("hotspot-detect.html") != std::string::npos) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody("<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
        callback(resp);
        return;
    }
    
    // Ubuntu connectivity check
    if (requestUri.find("connectivity-check") != std::string::npos) {
        auto resp = HttpResponse::newRedirectionResponse("http://192.168.4.1/");
        callback(resp);
        return;
    }
    
    // Default: redirect any other request to our main portal
    auto resp = HttpResponse::newRedirectionResponse("http://192.168.4.1/");
    resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    callback(resp);
}
