#include "apiResponse.h"

void sendRequestJson(AsyncWebServerRequest* request, JsonVariant payload, int code) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->setCode(code);
    serializeJson(payload, *response);
    request->send(response);
}