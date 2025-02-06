
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

/*
Fazer adaptação utilizando biblioteca #include "SPIFFS.h" 
para armazenar dados em arquivo interno no arduino
*/


const char* ssid = "DASS-CORP";
const char* password = "dass7425corp";
const char* serverName = "http://10.100.1.43:3041/porta-ti";
const char* offlinePassword = "94515670";
const unsigned long RECONNECT_INTERVAL = 10000;
unsigned long lastReconnectAttempt = 0;
bool offlineMode = false;
bool httpRequestInProgress = false;
bool offlineTest = false;
byte uidGlobal[10];
byte uidSize = 0;
unsigned long uidDecimal = 0;

#define RELAY_PIN 32
// Definições SPI para RFID
#define SS_PIN 5    // GPIO 5
#define RST_PIN 22  // GPIO 22

SPIClass SPI_OLED(HSPI);
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI_OLED, OLED_DC, OLED_RST, OLED_CS);

// Ícone de “check” (8×8)
const unsigned char check_icon[] PROGMEM = {
  B00000000,  // linha superior
  B00000001,
  B00000010,
  B00010100,
  B00101000,
  B01010000,
  B10000000,
  B00000000  // linha inferior
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

// Array com os valores RFID autorizados - Offline
unsigned long allowedRFIDs[] = {
  1701838869,  // HENDRIUS
  3047718818,  // ELIAS
  3298720930,  // SERGIO
  3047181186,  // EDILSOM
  2396287374,  // CLENERTY
  2410370222,  // MICHEL
  2396170782,  // MARCOS
  3625882750,  // RAMON
  2870376092,  // LAUZO
  3046097122,  // MARCIO
  1455116486,  // UILLIAM
  2674275395,  // GERAL
  486696609,   // GERAL 2
};
const int numRFIDs = sizeof(allowedRFIDs) / sizeof(allowedRFIDs[0]);

bool isRFIDAllowed(unsigned long rfidValue) {
  for (int i = 0; i < numRFIDs; i++) {
    if (rfidValue == allowedRFIDs[i]) {
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(SS_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  SPI_OLED.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Falha ao inicializar a tela OLED"));
    for (;;)
      ;
  }
  Serial.println(F("Tela OLED inicializada"));

  updateDisplay("Configurando", "Porta RFID");
  delay(2500);
  updateDisplay("Conectando", "WIFI...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando ao Wi-Fi...");
  }
  Serial.println("\nConectado ao Wi-Fi");
  updateDisplay("WIFI", "Conectado!");
  delay(1500);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && httpRequestInProgress == false && !offlineMode) {
    updateDisplay("Aproxime o Cracha", "Do Leitor");
  } else if (WiFi.status() != WL_CONNECTED) {
    offlineMode = true;
    updateDisplay("Sem Conexao", "Tente Novamente!");

    if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("Tentando reconectar ao Wi-Fi...");
      updateDisplay("Tentando reconectar", "WIFI!");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  } else if (offlineMode) {
    updateDisplay("Modo Offline", "");
  }

  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Armazenar o UID na variável global
  uidSize = rfid.uid.size;
  for (byte i = 0; i < uidSize; i++) {
    uidGlobal[i] = rfid.uid.uidByte[i];
  }

  // Mostrar UID no monitor serial em hexadecimal
  Serial.print("UID tag (hex): ");
  for (byte i = 0; i < uidSize; i++) {
    Serial.print(uidGlobal[i] < 0x10 ? " 0" : " ");
    Serial.print(uidGlobal[i], HEX);
  }
  Serial.println();

  if (!httpRequestInProgress) {
    checkRfid();
  }

  // Halt PICC
  rfid.PICC_HaltA();
}

unsigned long convertUID(uint8_t* uid, byte size) {
  unsigned long result = 0;
  // Monta o número: o primeiro byte (uid[0]) será o menos significativo
  for (int i = 0; i < size; i++) {
    result |= ((unsigned long)uid[i]) << (8 * i);
  }
  return result;
}

void checkRfid() {
  if (WiFi.status() == WL_CONNECTED) {
    updateDisplay("Procurando", "Funcionário...");
    delay(1000);

    // Converter o UID para uma string hexadecimal
    char uidString[uidSize * 2 + 1];
    for (byte i = 0; i < uidSize; i++) {
      sprintf(&uidString[i * 2], "%02X", uidGlobal[i]);
    }

    // Convertando em Decimal
    uidDecimal = convertUID(uidGlobal, uidSize);
    Serial.print("UID em decimal: ");
    Serial.println(uidDecimal);

    if (offlineMode) {
      offlineTest = true;
      updateDisplay("Procurando", "Memoria Interna...");
      delay(1000);
      if (isRFIDAllowed(uidDecimal)) {
        updateDisplayPermission("Colaborador", "Liberado!", true);

        digitalWrite(RELAY_PIN, HIGH);
        delay(2000);
        digitalWrite(RELAY_PIN, LOW);
      } else {
        updateDisplayPermission("Colaborador", "Nao Liberado!", false);
      }
    }

    offlineMode = false;
    httpRequestInProgress = true;

    char rfidData[100];
    snprintf(rfidData, sizeof(rfidData), "{\"rfid\": \"%lu\"}", uidDecimal);

    String serverResult;
    if (!offlineTest) {
      int code = sendHttpRequest(rfidData, serverResult);
      processHttpResponse(code, serverResult);
    }

    offlineTest = false;
    httpRequestInProgress = false;
    delay(1000);
  } else {
    offlineMode = true;

    if (isRFIDAllowed(uidDecimal)) {
      updateDisplayPermission("Colaborador", "Liberado!", true);

      digitalWrite(RELAY_PIN, HIGH);
      delay(2000);
      digitalWrite(RELAY_PIN, LOW);
    } else {
      updateDisplayPermission("Colaborador", "Nao Liberado!", false);
    }
  }
}

int sendHttpRequest(const char* rfidData, String& result) {
  WiFiClient client;
  HTTPClient http;

  http.setTimeout(4000);
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(rfidData);
  result = http.getString();

  http.end();

  return httpResponseCode;
}

void processHttpResponse(int httpResponseCode, const String& result) {
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, result);
  const char* nome = nullptr;

  if (!err) {
    nome = doc["nome"];
  }

  if (httpResponseCode < 0) {
    Serial.println("Modo Offline! ");
    Serial.println(httpResponseCode);
    offlineMode = true;

  } else if (httpResponseCode == 200) {
    Serial.println(nome ? nome : "(nome indefinido)");
    updateDisplayPermission(nome ? nome : "Colaborador", "Liberado!", true);
    
    // Aciona a porta
    digitalWrite(RELAY_PIN, HIGH);
    delay(2000);
    digitalWrite(RELAY_PIN, LOW);
  } else if (httpResponseCode == 404) {
    if (isRFIDAllowed(uidDecimal)) {
      Serial.println("RFID permitido!");
      updateDisplayPermission(nome ? nome : "Colaborador", "Liberado!", true);

      digitalWrite(RELAY_PIN, HIGH);
      delay(2000);
      digitalWrite(RELAY_PIN, LOW);
    } else {
      Serial.println("RFID não permitido!");
      updateDisplayPermission("Colaborador", "Nao Liberado!", false);
    }
  } else {
    updateDisplayPermission("Colaborador", "Nao Liberado!", false);
  }

  Serial.println(result);
  Serial.println(httpResponseCode);
}

void updateDisplay(String text, String text2) {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println(F("DASS - TI"));
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

  // Exemplo de retângulo e texto
  display.drawRect(0, 0, SCREEN_WIDTH, 30, SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println(F("DASS - TI"));

  // Texto principal
  display.setTextSize(1);
  display.setCursor(5, 45);
  display.println(text);

  // Texto secundário (opcional)
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