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
WebSocketClient wsClient(WEBSOCKET_SERVER, &storage, &actuator, &rfidModule);
WifiClient wifiClient(ssid, password, 30000);

void handleWebSocketCommand(const String &command, JsonDocument &doc)
{
  String response;
  if (command == "add_rfids")
  {
    bool add = wsClient.addRfid(doc, response);
    display.showMessage("Colaborador", add ? "adicionado!" : "nao adicionado!");
  }
  else if (command == "remove_rfid")
  {
    bool remove = wsClient.removeRfid(doc, response);
    display.showMessage("Colaborador", remove ? "removido!" : "nao removido!");
  }
  else if (command == "get_all")
  {
    wsClient.getAllRfid(doc, response);
    display.showMessage("Enviando", "Lista", 1500);
  }
  else if (command == "open_door")
  {
    wsClient.openDoor(doc, response);
    display.showMessage("Porta", "Aberta!", 1000);
  }
  else if (command == "get_access_history")
  {
    wsClient.getAccessHistory(doc, response);
    display.showMessage("Enviando", "Historico", 1500);
  }
  else
  {
    String client = doc["client"] | "";
    wsClient.sendErrorResponse(client, command, "Unknown command", response);
    return;
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
  
  // Initialize EEPROM
  EEPROM.begin(Storage::EEPROM_SIZE);
  Serial.println("EEPROM initialized");

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
