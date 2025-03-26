#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
#include <vector>
#include "SPIFFS.h"
#include <esp_task_wdt.h>

#define WDT_TIMEOUT_MS 120000

byte uidGlobal[10];
byte uidSize = 0;
unsigned long uidDecimal = 0;
unsigned long timeout = 30000;
unsigned long startTime = millis();

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

// Botão acionamento interno
const int botaoPin = 2;

const char *ssid = "your_ssid";
const char *password = "your_password";
const char *websocket_server = "ws://your_endpoint";
using namespace websockets;
bool websocketConnected = false;
unsigned long lastReconnectWifiAttempt = 0;
const unsigned long reconnectWifiInterval = 60000;
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 20000;
unsigned long lastPing = 0;
const unsigned long pingInterval = 40000;

SPIClass SPI_OLED(HSPI);
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI_OLED, OLED_DC, OLED_RST, OLED_CS);

std::vector<unsigned long> allowedRFIDs;

// Controle do relé via botão
unsigned long inicioContagem = 0;
bool executado = false;

// Flag para envio de mensagem ao client
bool answer_client = false;

bool timer(unsigned long inicio, unsigned long duracao) {
  return (millis() - inicio >= duracao);
}

// Variáveis para processamento do RFID
bool processingRFID = false;
bool rfidAuthorized = false;
unsigned long rfidTimerStart = 0;
unsigned long rfidDisplayDuration = 0;

WebsocketsClient client;
void setup() {
  Serial.begin(115200);
  pinMode(SS_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(botaoPin, INPUT_PULLUP);
  digitalWrite(SS_PIN, HIGH);

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_MS,
    .trigger_panic = true  // Se verdadeiro, o sistema entrará em modo panic em caso de timeout
  };
  esp_err_t ret = esp_task_wdt_init(&wdt_config);
  if (ret != ESP_OK) {
    Serial.println("Erro ao iniciar o Watchdog!");
  } else {
    Serial.println("Watchdog iniciado com sucesso.");
  }

  esp_task_wdt_add(NULL);

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  SPI_OLED.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Falha ao inicializar a tela OLED"));
    while (true)
      ;
  }
  Serial.println(F("Tela OLED inicializada"));
  updateDisplay("Configurando", "Porta RFID");
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    updateDisplay("Erro", "Chame Suporte. (COD-500)");
  } else {
    Serial.println("SPIFFS iniciado com sucesso");
    carregarRFIDsDoArquivo();
  }

  updateDisplay("Conectando", "WIFI...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime < timeout)) {
    delay(500);
    Serial.println("Conectando ao Wi-Fi...");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi");
    updateDisplay("WIFI", "Conectado!");
    websocketConfig();
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi");
    updateDisplay("WIFI", "FALHA!");
  }

  delay(500);
}

void loop() {
  // ----- Controle do botão para acionar o relé -----
  int estadoBotao = digitalRead(botaoPin);
  if (estadoBotao == LOW && !executado) {
    updateDisplay("Abrindo", "Porta");
    digitalWrite(RELAY_PIN, HIGH);
    inicioContagem = millis();
    executado = true;
  }
  if (executado && timer(inicioContagem, 2000)) {
    digitalWrite(RELAY_PIN, LOW);
    executado = false;
  }

  // ----- Gerenciamento do WebSocket e reconexão -----
  if (WiFi.status() == WL_CONNECTED) {
    if (websocketConnected) {
      updateDisplay("Aproxime", "O Cracha - Conectado");
      client.poll();

      if (answer_client) {
        client.send("ok");
      }
      answer_client = false;
    } else {
      updateDisplay("Aproxime", "O Cracha - Lost.");
      unsigned long now = millis();

      // Tenta reconectar ao WebSocket
      if (now - lastReconnectAttempt > reconnectInterval) {
        Serial.println("Tentando reconectar WebSocket...");
        websocketConfig();
        lastReconnectAttempt = now;
      }
    }

    // Keep-Alive check
    if (millis() - lastPing > pingInterval) {
      client.ping();
      lastPing = millis();
    }
  } else {
    updateDisplay("Modo Offline", "Aproxime o Cracha");
    unsigned long now = millis();

    // Tenta reconectar ao WiFi
    if (now - lastReconnectWifiAttempt > reconnectWifiInterval) {
      updateDisplay("Sem Conexao", "Tentando Reconectar");
      Serial.println("Tentando reconectar Wifi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      lastReconnectWifiAttempt = now;
    }
  }

  // ----- Processamento do RFID -----
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Armazenar o UID na variável global
    uidSize = rfid.uid.size;
    for (byte i = 0; i < uidSize; i++) {
      uidGlobal[i] = rfid.uid.uidByte[i];
    }
    checkRfid();
    rfid.PICC_HaltA();
  }

  // Verifica se o tempo do processamento RFID expirou
  if (processingRFID && (millis() - rfidTimerStart >= rfidDisplayDuration)) {
    if (rfidAuthorized) {
      digitalWrite(RELAY_PIN, LOW);
    }
    processingRFID = false;
  }

  esp_task_wdt_reset();
  delay(500);
}

void websocketConfig() {
  client.onMessage([](WebsocketsMessage message) {
    String data = message.data();
    answer_client = true;

    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (!error) {
      String command = doc["command"];

      if (command == "add_rfids") {
        JsonArray rfids = doc["rfids"];
        for (JsonVariant uid : rfids) {
          unsigned long rfidValue = uid.as<unsigned long>();
          if (!isRFIDAllowed(rfidValue)) {
            allowedRFIDs.push_back(rfidValue);
            Serial.print("Adicionado: ");
            Serial.println(rfidValue);
          }
        }

        updateDisplay("Adicionando", "Novo Colaborador!");
        salvarRFIDsNoArquivo();
        Serial.println("Lista atualizada com sucesso!");
        Serial.print("Total: ");
        Serial.println(allowedRFIDs.size());
      } else if (command == "remove-rfid") {
        unsigned long removeRfid = doc["rfid"];
        bool found = false;

        for (auto it = allowedRFIDs.begin(); it != allowedRFIDs.end(); ++it) {
          if (*it == removeRfid) {
            allowedRFIDs.erase(it);
            updateDisplay("Removendo", "Colaborador!");
            Serial.print("Removido: ");
            Serial.println(removeRfid);
            salvarRFIDsNoArquivo();
            found = true;
            break;
          }
        }

        if (!found) {
          Serial.print("RFID não encontrado: ");
          Serial.println(removeRfid);
        }
      } else if (command == "clear-list") {
        allowedRFIDs.clear();
        Serial.println("Lista limpa");
        Serial.println(allowedRFIDs.size());
        salvarRFIDsNoArquivo();
      } else if (command == "open") {
        updateDisplayPermission("Abrindo", "Porta!", true);
        digitalWrite(RELAY_PIN, HIGH);
        processingRFID = true;
        rfidAuthorized = true;
        rfidTimerStart = millis();
        rfidDisplayDuration = 2000;
      }
    } else {
      Serial.print("Erro ao decodificar JSON: ");
      Serial.println(error.c_str());
      updateDisplay("Erro ao decodificar", "Dados Recebidos!");
    }
  });

  client.onEvent([](WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
      websocketConnected = true;
      Serial.println("Conectado ao servidor WebSocket!");
      client.send("{\"nome\": \"porta_ti\"}");
    } else if (event == WebsocketsEvent::ConnectionClosed) {
      websocketConnected = false;
      Serial.println("Desconectado!");
    } else if (event == WebsocketsEvent::GotPing) {
      client.pong();
    }
  });

  client.connect(websocket_server);
  updateDisplay("Conectando", "Porta...");
}

bool isRFIDAllowed(unsigned long rfidValue) {
  for (unsigned long rfid : allowedRFIDs) {
    if (rfid == rfidValue)
      return true;
  }
  return false;
}

void checkRfid() {
  uidDecimal = convertUID(uidGlobal, uidSize);
  Serial.println("Procurando funcionário");
  updateDisplay("Procurando", "Funcionario...");

  if (isRFIDAllowed(uidDecimal)) {
    updateDisplayPermission("Colaborador", "Liberado!", true);
    digitalWrite(RELAY_PIN, HIGH);
    processingRFID = true;
    rfidAuthorized = true;
    rfidTimerStart = millis();
    rfidDisplayDuration = 2000;
  } else {
    updateDisplayPermission("Colaborador", "Nao Liberado!", false);
    processingRFID = true;
    rfidAuthorized = false;
    rfidTimerStart = millis();
    rfidDisplayDuration = 1000;
  }
}

void carregarRFIDsDoArquivo() {
  File file = SPIFFS.open("/rfids.json", FILE_READ);
  if (!file) {
    Serial.println("Arquivo de RFIDs não encontrado, usando lista padrão");
    return;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Erro ao ler JSON: ");
    Serial.println(error.c_str());
    updateDisplay("Erro", "Ao ler lista!");
    return;
  }

  allowedRFIDs.clear();
  JsonArray array = doc["rfids"];
  for (JsonVariant uid : array) {
    allowedRFIDs.push_back(uid.as<unsigned long>());
  }
  Serial.print("Lista carregada da memória. Total: ");
  Serial.println(allowedRFIDs.size());
}

void salvarRFIDsNoArquivo() {
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.createNestedArray("rfids");

  for (unsigned long rfid : allowedRFIDs) {
    array.add(rfid);
  }

  File file = SPIFFS.open("/rfids.json", FILE_WRITE);
  if (!file) {
    Serial.println("Erro ao abrir arquivo para escrita");
    updateDisplay("Erro ao atualizar", "Lista!");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Lista de RFIDs salva com sucesso!");
  updateDisplay("Lista atualizada", "Com Sucesso!");
}

unsigned long convertUID(uint8_t *uid, byte size) {
  unsigned long result = 0;
  for (int i = 0; i < size; i++) {
    result |= ((unsigned long)uid[i]) << (8 * i);
  }
  return result;
}

// Ícone de “check” (8×8)
const unsigned char check_icon[] PROGMEM = {
  B00000000,
  B00000001,
  B00000010,
  B00010100,
  B00101000,
  B01010000,
  B10000000,
  B00000000
};

// Ícone de “X” (8×8)
const unsigned char cross_icon[] PROGMEM = {
  B10000001,
  B01000010,
  B00100100,
  B00011000,
  B00011000,
  B00100100,
  B01000010,
  B10000001
};

void updateDisplay(String text, String text2) {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println(F("Tranca Inteligente"));
  display.setTextSize(1);
  display.setCursor(10, 45);
  display.println(text);
  if (text2.length() > 0) {
    display.setCursor(10, 55);
    display.println(text2);
  }
  display.display();
}

void updateDisplayPermission(String text, String text2, bool statusOK) {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println(F("Tranca Inteligente"));
  display.setTextSize(1);
  display.setCursor(5, 45);
  display.println(text);
  if (text2.length() > 0) {
    display.setCursor(5, 55);
    display.println(text2);
  }
  display.setTextSize(1.5);
  if (statusOK) {
    display.drawBitmap(110, 45, check_icon, 8, 8, SSD1306_WHITE);
  } else {
    display.drawBitmap(110, 45, cross_icon, 8, 8, SSD1306_WHITE);
  }
  display.display();
}
