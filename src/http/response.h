#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

void sendJson(WebServer& server, int code, const JsonDocument& body);
void sendError(WebServer& server, int code, const char* message);
