// #include "Config.h"
#include "RFIDModule.h"
#include "Storage.h"
// #include "WebSocketClient.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include "Display.h"
// #include "Actuator.h"

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

// Instâncias globais dos módulos
MFRC522 rfid(SS_PIN, RST_PIN);

SPIClass SPI_OLED(HSPI);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI_OLED, OLED_DC, OLED_RST, OLED_CS);
Display display(&oled);

Storage storage;
Actuator actuator(RELAY_PIN, BOTAO_PIN);
RFIDModule rfidModule(&rfid, &storage, &actuator);
// WebSocketClient wsClient(WEBSOCKET_SERVER, &storage);

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
  Serial.println(F("Configurando porta RFID"));
  delay(1000);

  // Inicialização dos módulos
  if (!storage.begin())
  {
    display.showMessage("Erro", "Falha ao inicializar armazenamento");
    return;
  }

  display.showMessage("Armazenamento", "Carregando dados");
  if (!storage.loadList())
  {
    display.showMessage("Erro", "Falha ao carregar dados");
    return;
  }

  display.showMessage("RFID", "Configurando Leitor...");
  SPI.begin(18, 19, 23, SS_PIN);
  rfidModule.begin();
  actuator.begin();
  // wsClient.begin();

  // Registrar callbacks se necessário
  rfidModule.setAccessCallback([](bool success, unsigned long cardId)
                               {
      display.showAccess(success);
      if(success) actuator.open(); });
  // wsClient.setCommandCallback([](const String& command, const JsonDocument& doc){
  //     // Delegar ao módulo Storage ou Actuator conforme comando
  // });
}

void loop()
{
  display.showMessage("Aproxime", "O Cracha");
  rfidModule.loop();
  actuator.loop();
  // wsClient.loop();
  delay(700);
}
