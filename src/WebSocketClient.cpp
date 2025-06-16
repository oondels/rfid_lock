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

void WebSocketClient::sendErrorResponse(const String &client, const String &command, const String &errorMsg, String &response)
{
  StaticJsonDocument<128> respDoc;
  if (client != "")
    respDoc["callBack"]["client"] = client;
  if (command != "")
    respDoc["callBack"]["command"] = command;
  respDoc["error"] = errorMsg;
  respDoc["callBack"]["status"] = "error";
  serializeJson(respDoc, response);
}

bool WebSocketClient::addRfid(JsonDocument &doc, String &response)
{
  bool status = false;
  String client = doc["client"] | "";
  String command = doc["command"] | "add_rfids";
  if (!doc.containsKey("rfids"))
  {
    this->sendErrorResponse(client, command, "Missing rfids field", response);
    return status;
  }

  int added = storage->addRFIDs(doc);
  StaticJsonDocument<128> respDoc;
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;
  if (added <= 0)
  {
    respDoc["error"] = "Failed to add rfids";
    respDoc["callBack"]["status"] = "error";
  }
  else
  {
    respDoc["added"] = added;
    respDoc["callBack"]["status"] = "success";
    status = true;
  }

  serializeJson(respDoc, response);
  return status;
}

bool WebSocketClient::removeRfid(JsonDocument &doc, String &response)
{
  bool status = false;
  StaticJsonDocument<128> respDoc;

  String client = doc["client"] | "";
  String command = doc["command"] | "remove_rfid";
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;
  if (!doc.containsKey("rfid"))
  {
    this->sendErrorResponse(client, command, "Missing rfid field", response);
    return status;
  }

  unsigned long rfid = doc["rfid"].as<unsigned long>();
  int removeResult = storage->removeRFID(rfid);

  if (removeResult > 0)
  {
    respDoc["callBack"]["status"] = "success";
    status = true;
  }
  else
  {
    respDoc["callBack"]["status"] = "error";
    respDoc["error"] = "Colaborador nao removido!";
  }

  serializeJson(respDoc, response);
  return status;
}

void WebSocketClient::getAllRfid(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "get_all";
  StaticJsonDocument<128> respDoc;
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;

  std::vector<unsigned long> rfids_list = storage->getAll();
  // Cria array json para lista de rfids
  JsonArray rfidsArray = respDoc["callBack"]["rfids_list"].to<JsonArray>();
  // Adiciona cada rfid ao array
  for (unsigned long rfid : rfids_list)
  {
    rfidsArray.add(rfid);
  }

  serializeJson(respDoc, response);
}