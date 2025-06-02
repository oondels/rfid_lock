#include <WiFi.h>

class WifiClient
{
public:
  WifiClient(const char *ssid, const char *password, unsigned long timeout);
  void begin();
  bool loop();
  bool checkConnection();
  void reconnect();
  
private:
  const char *wifiSsid;
  const char *wifiPassword;
  unsigned long timeout;
};