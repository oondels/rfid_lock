#pragma once
#include <ArduinoWebsockets.h>
#include "Storage.h"
#include "Actuator.h"
#include "RFIDModule.h"
using namespace websockets;

class WebSocketClient
{
public:
  WebSocketClient(const char *server, Storage *storage, Actuator *actuator, RFIDModule *rfidModule);
  void begin();
  bool loop();
  void checkConnection();
  void sendHeartbeat();
  void setupEventHandlers();
  void setCommandCallback(void (*callback)(const String &command, JsonDocument &doc));
  void sendEvent(const String &eventJson);
  void sendErrorResponse(const String &client, const String &command, const String &errorMsg, String &response);
  bool addRfid(JsonDocument &doc, String &response);
  bool removeRfid(JsonDocument &doc, String &response);
  void getAllRfid(JsonDocument &doc, String &response);
  void openDoor(JsonDocument &doc, String &response);
  void getAccessHistory(JsonDocument &doc, String &response);

private:
  RFIDModule *rfidModule;
  WebsocketsClient client;
  const char *serverUrl;
  Storage *storage;
  Actuator *actuator;
  void (*commandCallback)(const String &, JsonDocument &) = nullptr;
  void (*statusCallback)(bool) = nullptr;
};
