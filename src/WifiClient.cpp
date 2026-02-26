#include "MyWifiClient.h"

const unsigned long reconnectWifiInterval = 60000;
unsigned long lastReconnectWifiAttempt = 0;

MyWifiClient::MyWifiClient(const char *ssid, const char *password, unsigned long timeout)
    : wifiSsid(ssid), wifiPassword(password), timeout(timeout) {}

void MyWifiClient::begin()
{
  WiFi.begin(wifiSsid, wifiPassword);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime < timeout))
  // while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.println("Conectando Wi-Fi...");
  }
  checkConnection();
}

bool MyWifiClient::checkConnection()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWi-fi conectado");
    return true;
  }
  else
  {
    Serial.println("\nFalha ao conectar Wi-Fi");
    return false;
  }
}

bool MyWifiClient::loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }
  return false;
}

void MyWifiClient::reconnect()
{
  unsigned long now = millis();

  if (now - lastReconnectWifiAttempt > reconnectWifiInterval)
  {
    Serial.println("Trying to reconnect wifi...");
    WiFi.disconnect();
    WiFi.begin(wifiSsid, wifiPassword);
    lastReconnectWifiAttempt = now;
  }
}