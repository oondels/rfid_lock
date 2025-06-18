# Project: Access Control with RFID and ESP32

## Description

This project uses an **Arduino ESP32** and an **RFID RC522 reader** for access control, allowing doors to be unlocked through RFID card authentication. The system also includes a **0.96'' SPI OLED display** to show information.

Additionally, an API collects and validates RFID data to verify access permissions.

---

## Technologies Used

<img src="https://github.com/oondels/rfid_lock/blob/main/images/Node.js_logo.svg.png?raw=true" width="200" height="100">
<img src="https://github.com/oondels/rfid_lock/blob/main/images/arduino.png?raw=true" width="200" height="100">

- Node.js
- C++
- SQL
- Postgres

---

## Hardware Used

- **ESP32** (Main Board)
- **RFID-RC522** (RFID Reader)
- **0.96'' SPI OLED Display** (For displaying messages)
- **Relay or something else** (To activate the door)

---

## Connections

### **RFID-RC522**

| Pin      | ESP32   |
| -------- | ------- |
| SDA (SS) | GPIO 5  |
| SCK      | GPIO 18 |
| MOSI     | GPIO 23 |
| MISO     | GPIO 19 |
| GND      | GND     |
| 3.3V     | 3.3V    |
| RST      | GPIO 22 |

### **SPI OLED Display**

| Pin        | ESP32   |
| ---------- | ------- |
| VCC        | 3.3V    |
| GND        | GND     |
| SCL (SCK)  | GPIO 14 |
| SDA (MOSI) | GPIO 13 |
| RST        | GPIO 16 |
| D/C        | GPIO 17 |
| CS         | GPIO 4  |

### **Relay**

| Pin    | ESP32   |
| ------ | ------- |
| Signal | GPIO 32 |

---

## Libraries Used

For the system to function properly, install the following libraries in the Arduino IDE:

- **WiFi.h**
- **HTTPClient.h**
- **ArduinoJson.h**
- **SPI.h**
- **MFRC522.h**
- **Adafruit_GFX.h**
- **Adafruit_SSD1306.h**

---

## Installation and Setup

1. Connect the components according to the wiring diagram above.
2. Install the mentioned libraries in the Arduino IDE.
3. Update the Wi-Fi credentials in the code:

   ```cpp
   cpp
   CopiarEditar
   const char* ssid = "your_ssid";
   const char* password = "your_password";

   ```

4. Configure the API for RFID card validation:

   ```cpp
   cpp
   CopiarEditar
   const char* serverName = "<YO,UR_API>";

   ```

5. Compile and upload the code to the ESP32.

---

## How It Works

1. The ESP32 starts and connects to Wi-Fi.
2. The OLED display shows **"Approach Your Badge"**.
3. The RFID-RC522 detects the card and converts its UID to decimal.
4. The UID is sent to a server via **HTTP request**.
5. The API responds, indicating whether the RFID is allowed.
6. If allowed, the relay is triggered, unlocking the door.
7. Otherwise, the screen displays "Access Denied."
8. In case of connection failure, the system enters **offline mode** and checks a locally stored list of authorized cards.

---

## Future Improvements

- Implement a **remote database** for managing RFID cards.
- Add a **more efficient offline backup mechanism**.
- Improve the OLED screen interface with interactive menus.

---

## Author

- **Name:** Hendrius Félix Cerqueira Gomes de Santana
- **Contact:** [GitHub](https://github.com/oondels)
