#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class CaptivePortalController : public HttpController<CaptivePortalController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CaptivePortalController::index, "/", Get);
    ADD_METHOD_TO(CaptivePortalController::index, "/index.html", Get);
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/generate_204", Get);
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/connecttest.txt", Get);
    ADD_METHOD_TO(CaptivePortalController::captiveDetect, "/redirect", Get);
    METHOD_LIST_END
    
    void index(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void captiveDetect(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
