#include "http/response.h"

void sendJson(WebServer& server, int code, const JsonDocument& body) {
  String text;
  text.reserve(measureJson(body));
  serializeJson(body, text);
  server.send(code, "application/json", text);
}

void sendError(WebServer& server, int code, const char* message) {
  JsonDocument body;
  body["ok"] = false;
  body["error"] = message;
  sendJson(server, code, body);
}
