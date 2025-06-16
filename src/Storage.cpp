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
  return std::find(allowedRFIDs.begin(), allowedRFIDs.end(), cardId) != allowedRFIDs.end();
}

int Storage::addRFIDs(JsonDocument &doc)
{
  if (!doc.containsKey("rfids") || !doc["rfids"].is<JsonArray>())
  {
    return -1;
  }

  JsonArray rfids = doc["rfids"].as<JsonArray>();
  int added = 0;
  for (JsonVariant uid : rfids)
  {
    unsigned long rfidValue = uid.as<unsigned long>();
    if (!isAllowed(rfidValue))
    {
      allowedRFIDs.push_back(rfidValue);
      Serial.print("Added rfid: ");
      Serial.println(rfidValue);
      added++;
    }
  }

  // Salva lista na memória
  if (!this->saveList())
  {
    return -1;
  }

  return added;
}

int Storage::removeRFID(unsigned long id)
{
  int count = 0;
  for (auto it = allowedRFIDs.begin(); it != allowedRFIDs.end();)
  {
    if (*it == id)
    {
      it = allowedRFIDs.erase(it); // erase retorna o próximo iterador válido
      count++;
    }
    else
    {
      ++it;
    }
  }

  // Salva lista na memória
  if (count > 0 && !this->saveList())
  {
    Serial.println("ERRO: Falha ao salvar lista!");
    return -1;
  }

  return count;
}

bool Storage::saveList()
{
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.createNestedArray("rfids");

  for (unsigned long rfid : allowedRFIDs)
  {
    array.add(rfid);
  }

  File file = SPIFFS.open("/rfids.json", FILE_WRITE);
  if (!file)
  {
    Serial.println("Erro ao abrir arquivo para escrita");
    return false;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Lista de RFIDs salva com sucesso!");
  return true;
}

std::vector<unsigned long> Storage::getAll()
{
  std::vector<unsigned long> rfids;

  File file = SPIFFS.open("/rfids.json", FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open rfid.json");
    return rfids;
  }

  String content = file.readString();
  file.close();
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, content);
  JsonArray array = doc["rfids"].as<JsonArray>();

  for (JsonVariant value : array)
  {
    rfids.push_back(value.as<unsigned long>());
  }
  return rfids;
}