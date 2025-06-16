#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// #include "Config.h"
#include "WebSocketClient.h"
#include "RFIDModule.h"
#include "Storage.h"
#include "Display.h"
#include "Actuator.h"
#include "WifiClient.h"

// Definições Tela Oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 17
#define OLED_RST 16
#define OLED_CS 4
#define OLED_SCK 14
#define OLED_MOSI 13

// Definições SPI para RFID
#define SS_PIN 5
#define RST_PIN 22
#define RELAY_PIN 32
#define BOTAO_PIN 2

// Wifi & Serve connection
const char *ssid = "DASS-CORP";
const char *password = "dass7425corp";
const char *WEBSOCKET_SERVER = "ws://10.110.21.105:3010";

// Instâncias globais dos módulos
MFRC522 rfid(SS_PIN, RST_PIN);
SPIClass SPI_OLED(HSPI);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI_OLED, OLED_DC, OLED_RST, OLED_CS);
Display display(&oled);
Storage storage;
Actuator actuator(RELAY_PIN, BOTAO_PIN);
RFIDModule rfidModule(&rfid, &storage, &actuator);
WebSocketClient wsClient(WEBSOCKET_SERVER, &storage);
WifiClient wifiClient(ssid, password, 30000);

bool checkCommandData(JsonDocument &doc, String param)
{
  StaticJsonDocument<128> respDoc;
  if (!doc.containsKey("client") || !doc.containsKey(param))
  {
    respDoc["error"] = "Invalid Command Payload";
    return false;
  }
  return true;
}

// --- Funções de tratamento de comandos WebSocket ---
void sendErrorResponse(const String &client, const String &command, const String &errorMsg, String &response)
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

void handleAddRfidsCommand(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "add_rfids";
  if (!doc.containsKey("rfids"))
  {
    sendErrorResponse(client, command, "Missing rfids field", response);
    return;
  }

  int added = storage.addRFIDs(doc);
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
    display.showMessage("Colaborador", "adicionado!");
    delay(1000);
  }
  serializeJson(respDoc, response);
}

void handleRemoveRfidCommand(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "remove_rfid";
  if (!doc.containsKey("rfid"))
  {
    sendErrorResponse(client, command, "Missing rfid field", response);
    return;
  }

  unsigned long rfid = doc["rfid"].as<unsigned long>();
  if (storage.removeRFID(rfid) > 0)
  {
    StaticJsonDocument<128> respDoc;
    respDoc["callBack"]["client"] = client;
    respDoc["callBack"]["command"] = command;
    respDoc["callBack"]["status"] = "success";
    display.showMessage("Colaborador", "removido!");
    serializeJson(respDoc, response);
  }
  else
  {
    sendErrorResponse(client, command, "Colaborador nao removido!", response);
    display.showMessage("Erro", "Nao removido!");
  }
  delay(1000);
}

void handleGetAllCommand(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "get_all";

  std::vector<unsigned long> rfids_list = storage.getAll();
  StaticJsonDocument<128> respDoc;
  respDoc["callback"]["client"] = client;
  respDoc["callback"]["command"] = command;

  // Cria array json para lista de rfids
  JsonArray rfidsArray = respDoc["callback"]["rfids_list"].to<JsonArray>();
  // Adiciona cada rfid ao array
  for (unsigned long rfid: rfids_list)
  {
    rfidsArray.add(rfid);
  }

  display.showMessage("Enviando", "Lista", 1500);
  serializeJson(respDoc, response);
}

void handleWebSocketCommand(const String &command, JsonDocument &doc)
{
  String response;
  if (command == "add_rfids")
  {
    handleAddRfidsCommand(doc, response);
  }
  else if (command == "remove_rfid")
  {
    handleRemoveRfidCommand(doc, response);
  }
  else if (command == "get_all")
  {
    handleGetAllCommand(doc, response);
  }
  else
  {
    String client = doc["client"] | "";
    sendErrorResponse(client, command, "Unknown command", response);
  }
  wsClient.sendEvent(response);
}

void setup()
{
  Serial.begin(115200);

  SPI_OLED.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!oled.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("Falha ao inicializar a tela OLED"));
    while (true)
      ;
  }

  display.showMessage("Configurando", "Porta RFID...");
  delay(700);

  // Inicialização dos módulos
  if (!storage.begin())
  {
    display.showMessage("Erro", "Falha ao inicializar armazenamento");
    return;
  }

  display.showMessage("Armazenamento", "Carregando dados");
  delay(500);
  if (!storage.loadList())
  {
    display.showMessage("Erro", "Falha ao carregar dados");
    return;
  }

  display.showMessage("WIFI", "Conectando Wifi");
  wifiClient.begin();
  display.showMessage("WIFI", wifiClient.checkConnection() ? "Conexao Estabelecida" : "Falha na conexao");
  delay(500);

  display.showMessage("RFID", "Configuracoes Finais...");
  delay(500);
  SPI.begin(18, 19, 23, SS_PIN);
  rfidModule.begin();
  actuator.begin();

  wsClient.begin();

  // Registrar callbacks se necessário
  rfidModule.setAccessCallback([](bool sucesso, unsigned long cardId)
                               { display.showAccess(sucesso, "", 1000); });

  // Callback centralizado e padronizado para comandos WebSocket
  wsClient.setCommandCallback(handleWebSocketCommand);
}

void loop()
{
  rfidModule.loop();
  actuator.loop();
  bool wsStatus = wsClient.loop();
  bool wifiStatus = wifiClient.loop();

  display.defaultMessage(wifiStatus, wsStatus);

  if (!wifiStatus)
  {
    wifiClient.reconnect();
  }
  delay(100);
}
