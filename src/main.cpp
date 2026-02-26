#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// #include "Config.h"
#include "WebSocketClient.h"
#include "RFIDModule.h"
#include "Storage.h"
#include "Display.h"
#include "Actuator.h"
#include "MyWifiClient.h"

// Definições Tela Oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 27
#define OLED_RST 26
#define OLED_CS 4
#define OLED_SCK 14
#define OLED_MOSI 13

// Definições SPI para RFID
#define SS_PIN 4
#define RST_PIN 2

#define RELAY_PIN 32
#define BOTAO_PIN 5

// Wifi & Serve connection
const char *ssid = "DASS-CORP";
const char *password = "dass7425corp";
const char *WEBSOCKET_SERVER = "ws://10.100.1.43:3010";

const char *door_name = "porta_ti";

// Instâncias globais dos módulos
MFRC522 rfid(SS_PIN, RST_PIN);
SPIClass SPI_OLED(HSPI);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI_OLED, OLED_DC, OLED_RST, OLED_CS);
Display display(&oled);
Storage storage;
Actuator actuator(RELAY_PIN, BOTAO_PIN);
RFIDModule rfidModule(&rfid, &storage, &actuator);
WebSocketClient wsClient(door_name, WEBSOCKET_SERVER, &storage, &actuator, &rfidModule);
WifiClient wifiClient(ssid, password, 20000);

// void handleWebSocketCommand(const String &command, JsonDocument &doc)
// {
//   String response;
//   if (command == "add_rfids")
//   {
//     bool add = wsClient.addRfid(doc, response);
//     display.showMessage("Colaborador", add ? "adicionado!" : "nao adicionado!");
//   }
//   else if (command == "remove_rfid")
//   {
//     bool remove = wsClient.removeRfid(doc, response);
//     display.showMessage("Colaborador", remove ? "removido!" : "nao removido!");
//   }
//   else if (command == "get_all")
//   {
//     wsClient.getAllRfid(doc, response);
//     display.showMessage("Enviando", "Lista", 1500);
//   }
//   else if (command == "open_door")
//   {
//     wsClient.openDoor(doc, response);
//   }
//   else if (command == "get_access_history")
//   {
//     wsClient.getAccessHistory(doc, response);
//   }
//   else if (command == "clear_history")
//   {
//     rfidModule.clearAccessHistory(doc, response);
//   }
//   else
//   {
//     String client = doc["client"] | "";
//     wsClient.sendErrorResponse(client, command, "Unknown command", response);
//     return;
//   }
//   wsClient.sendEvent(response);
// }

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");
  // Inicializa o atuador o mais cedo possível para garantir relé desligado
  actuator.begin();

  SPI_OLED.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!oled.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("Falha ao inicializar a tela OLED"));
    while (true)
      ;
  }
  Serial.println("Tela Oled");

  // Initialize EEPROM
  EEPROM.begin(Storage::EEPROM_SIZE);
  Serial.println("EEPROM initialized");

  display.showMessage("Configurando", "Porta RFID...");
  delay(300);

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

  // Clear all stored data
  // storage.clearMemory();

  // Add Rfids
  unsigned long allowedRFIDsArray[] = {
      2269219895, // Hendrius
      3625882750, // Ramon
      3416347418, // Guedes
      2617777157, // Miqueias
      1455116486, // Uilliam
      3628015726, // Bertolino 
      2629318421, // Hellen
      2409447214, // Leone
      3298720930, // Sergio 
      3047181186, // Edilson
      3046795490, // Renilson - Café 
      377341517, // Victor Eletricista
      165907952, // Ramon - Café
      2773097371, // Marcos,
      4125357762, // João Victor
      2619281925, // Carla Costa Limpeza
      2674278899, // Tag azul extra
      3625660014, // Andre, portaria
      3626186062, // portaria 01
      3624067710, // portaria 02
      2618688053, // Joseane
    };

  StaticJsonDocument<JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(20)> doc;
  JsonArray arr = doc.createNestedArray("rfids");
  for (auto rfid : allowedRFIDsArray)
    arr.add(rfid);

  int added = storage.addRFIDs(doc); // persists internally
  Serial.printf("Added %d new RFIDs\n", added);

  // display.showMessage("WIFI", "Conectando Wifi");
  // wifiClient.begin();
  // display.showMessage("WIFI", wifiClient.checkConnection() ? "Conexao Estabelecida" : "Falha na conexao");
  // delay(500);

  display.showMessage("RFID", "Configuracoes Finais...");
  SPI.begin(18, 19, 23, SS_PIN);
  rfidModule.begin();
  Serial.println("RFID initialized");

  // wsClient.setStatusCallback([](bool connected){});
  // wsClient.begin();

  // // Callback centralizado e padronizado para comandos WebSocket
  // wsClient.setCommandCallback(handleWebSocketCommand);

  // // Registrar callbacks se necessário
  rfidModule.setAccessCallback([](bool sucesso, unsigned long cardId)
                               { display.showAccess(sucesso, "", 1000); });
}

void loop()
{
  rfidModule.loop();
  actuator.loop();

  // bool wifiStatus = wifiClient.loop();
  // bool wsStatus = wsClient.loop();
  // if (!wifiStatus)
  // {
  //   wifiClient.reconnect();
  // }

  // display.defaultMessage(wifiStatus, wsStatus);

  display.defaultMessageOff();
  delay(100);
}

// #include <SPI.h>
// #include <MFRC522.h>

// // Pinos definidos por você
// #define SS_PIN 4
// #define RST_PIN 2

// MFRC522 rfid(SS_PIN, RST_PIN);

// void setup() {
//   Serial.begin(115200);
//   pinMode(SS_PIN, OUTPUT);
//   digitalWrite(SS_PIN, HIGH);

//   SPI.begin(18, 19, 23, SS_PIN);  // SCK=18, MISO=19, MOSI=23
//   rfid.PCD_Init();
//   Serial.println("Aproxime o cartao RFID...");
// }

// void loop() {
//   // Verifica se tem novo cartão presente
//   if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
//     delay(50);
//     return;
//   }

//   // Imprime o UID
//   Serial.print("UID do cartao: ");
//   for (byte i = 0; i < rfid.uid.size; i++) {
//     Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
//     Serial.print(rfid.uid.uidByte[i], HEX);
//     Serial.print(" ");
//   }
//   Serial.println();

//   // Para leitura contínua
//   rfid.PICC_HaltA();
// }
