// #pragma once
// #include <ArduinoWebsockets.h>
// using namespace websockets;

// class WebSocketClient
// {
// public:
//   WebSocketClient(const char *server, Storage *storage);
//   void begin();
//   void loop();
//   void sendEvent(const String &eventJson);
//   void setCommandCallback(void (*callback)(const String &command, const JsonDocument &doc));

// private:
//   WebsocketsClient client;
//   const char *serverUrl;
//   Storage *storage;
//   void (*commandCallback)(const String &, const JsonDocument &) = nullptr;
//   // Reconnect, ping, etc.
// };
