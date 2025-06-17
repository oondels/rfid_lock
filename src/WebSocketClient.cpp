#include "WebSocketClient.h"

// Connection parameters
unsigned long lastHeartBeat = 0;
const unsigned long heartbeatInterval = 8000;
const unsigned long connectionTimeout = 15000;
const unsigned long reconnectInterval = 5000;
const unsigned long connectionAttemptTimeout = 10000; // Timeout for connection attempts
bool websocketConnected = false;
bool aswerEvent = false;
unsigned long lastServerResponse = 0;
unsigned long connectionStartTime = 0;

WebSocketClient::WebSocketClient(const char *server, Storage *storage, Actuator *actuator, RFIDModule *rfidModule)
    : serverUrl(server), storage(storage), actuator(actuator), rfidModule(rfidModule) {
  // Setup event handlers once during construction
  setupEventHandlers();
}

void WebSocketClient::setupEventHandlers() {
  client.onEvent([this](WebsocketsEvent event, String data) {
    unsigned long now = millis();

    if (event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("Ws connected.");
      websocketConnected = true;
      lastServerResponse = now;
      sendEvent("{\"nome\": \"porta_ti\"}");
    } else if (event == WebsocketsEvent::ConnectionClosed) {
      Serial.println("Ws not connected.");
      websocketConnected = false;
    } else if (event == WebsocketsEvent::GotPing) {
      lastServerResponse = now;
      client.pong();
    }
  });
}

void WebSocketClient::begin() {
  // Only try to connect, event handlers are already set in constructor
  if (!websocketConnected) {
    Serial.println("Initiating connection to WebSocket server...");
    connectionStartTime = millis();
    client.connect(serverUrl);
    aswerEvent = true;
  }
}

bool WebSocketClient::loop() {
  client.poll();
  unsigned long now = millis();

  if (!websocketConnected && (now - connectionStartTime > connectionAttemptTimeout) && connectionStartTime > 0) {
    Serial.println("Connection attempt timed out, cleaning up...");
    client.close();
    connectionStartTime = 0;
  }
  
  if (websocketConnected && (now - lastServerResponse > connectionTimeout)) {
    Serial.println("Connection timeout, disconnecting...");
    client.close();
    websocketConnected = false;
  }

  // Handle reconnection
  if (!websocketConnected) {
    static unsigned long lastReconnectAttempt = 0;
    
    if (now - lastReconnectAttempt > reconnectInterval) {
      Serial.println("Attempting to reconnect to WebSocket server...");

      client.close(); 
      delay(100);
      
      // client = WebsocketsClient(); 
      setupEventHandlers();
      
      client.connect(serverUrl);
      aswerEvent = true;
      connectionStartTime = now;
      lastReconnectAttempt = now;
    }
  } else {
    if (now - lastHeartBeat > heartbeatInterval) {
      sendHeartbeat();
      lastHeartBeat = now;
    }
  }

  if (aswerEvent && websocketConnected) {
    sendEvent("{\"status\": \"ok\"}");
    aswerEvent = false;
  }

  if (statusCallback) {
    statusCallback(websocketConnected);
  }

  return websocketConnected;
}

void WebSocketClient::checkConnection() {
  if (statusCallback) {
    statusCallback(websocketConnected);
  }
}

void WebSocketClient::sendEvent(const String &eventJson) {
  if (websocketConnected) {
    client.send(eventJson);
  } else {
    Serial.println("Cannot send event: not connected");
  }
}

void WebSocketClient::setCommandCallback(void (*callback)(const String &command, JsonDocument &doc)) {
  commandCallback = callback;

  // Register the message handler
  client.onMessage([this](WebsocketsMessage message) {
    lastServerResponse = millis();
                  
    if (!commandCallback) {
      Serial.println("No command callback set, ignoring message");
      return;
    }
    
    StaticJsonDocument<512> doc;
    String messageData = message.data().c_str();
    DeserializationError error = deserializeJson(doc, messageData);
    if (error) {
      Serial.println("Failed to parse WebSocket message as JSON");
      return;
    }

    // Handle heartbeat acknowledgment
    String messageType = doc["type"] | "";
    if (messageType == "heartbeat_ack") {
      // Server acknowledged our heartbeat
      return;
    }

    String command = doc["command"] | "";
    commandCallback(command, doc);
  });
}

void WebSocketClient::sendHeartbeat() {
  if (websocketConnected) {
    StaticJsonDocument<64> heartbeatDoc;
    heartbeatDoc["type"] = "heartbeat";
    heartbeatDoc["timestamp"] = millis();
    heartbeatDoc["client"] = "porta_ti";
    
    String heartbeatMsg;
    serializeJson(heartbeatDoc, heartbeatMsg);
    client.send(heartbeatMsg);
  }
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

void WebSocketClient::openDoor(JsonDocument &doc, String &response)
{
  Serial.println();
  String client = doc["client"] | "";
  String command = doc["command"] | "open_door";
  StaticJsonDocument<128> respDoc;
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;

  actuator->open();
  respDoc["callBack"]["status"] = "success";

  serializeJson(respDoc, response);
}

void WebSocketClient::getAccessHistory(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "get_access_history";
  StaticJsonDocument<256> respDoc;
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;

  std::vector<unsigned long> accessHistory = rfidModule->getLastAccesses();
  unsigned long lastCardId = rfidModule->getLastAccessedCardId();
  
  // Create array for access history
  JsonArray historyArray = respDoc["callBack"]["access_history"].to<JsonArray>();
  for (unsigned long cardId : accessHistory)
  {
    historyArray.add(cardId);
  }
  
  respDoc["callBack"]["last_accessed_card"] = lastCardId;
  respDoc["callBack"]["status"] = "success";

  serializeJson(respDoc, response);
}

void WebSocketClient::setStatusCallback(void (*callback)(bool)) {
  statusCallback = callback;
}