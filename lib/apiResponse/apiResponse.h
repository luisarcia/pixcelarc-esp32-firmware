#ifndef APIRESPONSE_H
#define APIRESPONSE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

void sendRequestJson(AsyncWebServerRequest* request, JsonVariant payload, int code = 200); 

#endif