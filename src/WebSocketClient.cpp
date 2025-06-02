#include "WebSocketClient.h"

unsigned long lastPing = 0;
const unsigned long pingInterval = 40000;
bool websocketConnected = false;
bool aswerEvent = false;

WebSocketClient::WebSocketClient(const char *server, Storage *storage)
    : serverUrl(server), storage(storage) {}

void WebSocketClient::begin()
{
  checkConnection(); // Ensure event handler is set before connecting
  client.connect(serverUrl);
  aswerEvent = true;
}

bool WebSocketClient::loop()
{
  client.poll();
  checkConnection();
  if (!websocketConnected)
  {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000)
    { // Try every 5 seconds
      client.connect(serverUrl);
      lastReconnectAttempt = now;
    }
  }

  // Asnwer the server back in first loop
  if (aswerEvent)
  {
    sendEvent("{\"status\": \"ok\"}");
    aswerEvent = false;
  }

  return websocketConnected;
}

void WebSocketClient::checkConnection()
{
  client.onEvent([this](WebsocketsEvent event, String data)
                 {
    websocketConnected = true;
    if (event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("Ws connected.");
      sendEvent("{\"nome\": \"porta_ti\"}");
    } else if (event == WebsocketsEvent::ConnectionClosed) {
      Serial.println("Ws not connected.");
      websocketConnected = false;
    } else if (event == WebsocketsEvent::GotPing) {
      client.pong();
    } });

  // Set callback to update display
  if (statusCallback)
  {
    statusCallback(websocketConnected);
  }
}

void WebSocketClient::sendEvent(const String &eventJson)
{
  client.send(eventJson);
}

void WebSocketClient::setCommandCallback(void (*callback)(const String &command, JsonDocument &doc))
{
  commandCallback = callback;

  // Register the message handler
  client.onMessage([this](WebsocketsMessage message)
                   {
    if (!commandCallback) {
      return;
    }
    
    StaticJsonDocument<512> doc;
    String messageData = message.data().c_str();
    DeserializationError error = deserializeJson(doc, messageData);
    if (error) {
      Serial.println("Failed to parse WebSocket message as JSON");
      return;
    }

    String command = doc["command"] | "";
    commandCallback(command, doc); });
}