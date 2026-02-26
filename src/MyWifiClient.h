#include <WiFi.h>

class MyWifiClient
{
public:
  MyWifiClient(const char *ssid, const char *password, unsigned long timeout);
  void begin();
  bool loop();
  bool checkConnection();
  void reconnect();
  
private:
  const char *wifiSsid;
  const char *wifiPassword;
  unsigned long timeout;
};