#include "Storage.h"
#include <vector>

std::vector<unsigned long> allowedRFIDs;
Storage::Storage() {}
bool Storage::begin()
{
  return SPIFFS.begin(true);
}

bool Storage::loadList()

{
  if (!SPIFFS.exists("/rfids.json"))
  {
    return false;
  }

  File file = SPIFFS.open("/rfids.json", FILE_READ);
  if (!file)
  {
    return false;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    return false;
  }

  allowedRFIDs.clear();
  JsonArray rfids = doc["rfids"].as<JsonArray>();
  for (JsonVariant uid : rfids)
  {
    allowedRFIDs.push_back(uid.as<unsigned long>());
  }

  unsigned long defaultRfid = 2269219895;
  if (std::find(allowedRFIDs.begin(), allowedRFIDs.end(), defaultRfid) == allowedRFIDs.end())
  {
    allowedRFIDs.push_back(defaultRfid);
  }

  return true;
}

bool Storage::isAllowed(unsigned long cardId)
{
  Serial.println("Cartao Lido: ");
  Serial.println(cardId);
  return std::find(allowedRFIDs.begin(), allowedRFIDs.end(), cardId) != allowedRFIDs.end();
}