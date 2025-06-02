#pragma once
#include <ArduinoWebsockets.h>
#include "Storage.h"
using namespace websockets;

class WebSocketClient
{
public:
  WebSocketClient(const char *server, Storage *storage);
  void begin();
  bool loop();
  void checkConnection();
  void sendEvent(const String &eventJson);
  void setCommandCallback(void (*callback)(const String &command, JsonDocument &doc));

private:
  WebsocketsClient client;
  const char *serverUrl;
  Storage *storage;
  void (*commandCallback)(const String &, JsonDocument &) = nullptr;
  void (*statusCallback)(bool) = nullptr;
};
