#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

#include <optional>

// Parses a JSON body whose root is a `Root` (JsonObjectConst or JsonArrayConst)
// into `body`; no body reads as {} for an object. Sends 400 and returns false
// for anything else, query parameters and form fields included.
template <typename Root = JsonObjectConst>
bool readJsonBody(WebServer& server, JsonDocument& body);

struct ServoMove {
  int index;
  float angle;

  // Sends 400 and returns nullopt when the body holds no valid move. A move that
  // passes is one the servo driver accepts: the index is the only thing it checks.
  static std::optional<ServoMove> read(WebServer& server, JsonObjectConst body);

  void addTo(JsonDocument& reply) const;
};
