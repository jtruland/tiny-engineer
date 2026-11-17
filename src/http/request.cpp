#include "http/request.h"

#include <type_traits>

#include "http/response.h"
#include "servos.h"

template <typename Root>
bool readJsonBody(WebServer& server, JsonDocument& body) {
  constexpr bool kObject = std::is_same_v<Root, JsonObjectConst>;

  if (!server.hasArg("plain")) {
    // WebServer keeps "plain" only for bodies that are not form-encoded.
    if (server.args() > 0 || server.clientContentLength() > 0 || !kObject) {
      sendError(server, 400, "send a JSON body with Content-Type: application/json");
      return false;
    }

    body.to<JsonObject>();
    return true;
  }

  if (deserializeJson(body, server.arg("plain")) || !body.is<Root>()) {
    sendError(server, 400, kObject ? "body must be a JSON object" : "body must be a JSON array");
    return false;
  }

  return true;
}

template bool readJsonBody<JsonObjectConst>(WebServer& server, JsonDocument& body);
template bool readJsonBody<JsonArrayConst>(WebServer& server, JsonDocument& body);

std::optional<ServoMove> ServoMove::read(WebServer& server, JsonObjectConst body) {
  const JsonVariantConst index = body["index"];
  const JsonVariantConst angle = body["angle"];

  if (!index.is<int>() || index.as<int>() < 0 || index.as<int>() >= SERVO_COUNT) {
    sendError(server, 400, "invalid index");
    return std::nullopt;
  }

  if (!angle.is<float>() || angle.as<float>() < 0.0f || angle.as<float>() > 180.0f) {
    sendError(server, 400, "invalid angle");
    return std::nullopt;
  }

  return ServoMove{index.as<int>(), angle.as<float>()};
}

void ServoMove::addTo(JsonDocument& reply) const {
  reply["index"] = index;
  reply["angle"] = angle;
}
