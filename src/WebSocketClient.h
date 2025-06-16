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
  void setCommandCallback(void (*callback)(const String &command, JsonDocument &doc));
  void sendEvent(const String &eventJson);
  void sendErrorResponse(const String &client, const String &command, const String &errorMsg, String &response);
  bool addRfid(JsonDocument &doc, String &response);
  bool removeRfid(JsonDocument &doc, String &response);
  void getAllRfid(JsonDocument &doc, String &response);

private:
  WebsocketsClient client;
  const char *serverUrl;
  Storage *storage;
  void (*commandCallback)(const String &, JsonDocument &) = nullptr;
  void (*statusCallback)(bool) = nullptr;
};
