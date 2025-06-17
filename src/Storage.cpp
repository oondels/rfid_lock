#include "Storage.h"
#include <vector>

std::vector<unsigned long> allowedRFIDs;
Storage::Storage() {}
bool Storage::begin()
{
  if (!LittleFS.begin(/*formatOnFail=*/true))
  {
    Serial.println("Falha ao montar LittleFS");
    return false;
  }
  Serial.println("LittleFS montado com sucesso");
  return true;
}

bool Storage::loadList()
{
  if (!LittleFS.exists("/rfids.json"))
  {
    Serial.println("Arquivo rfids.json não encontrado, criando novo arquivo com RFID padrão.");
    File f = LittleFS.open("/rfids.json", FILE_WRITE);
    f.print("{\"rfids\":[]}");
    f.close();
  }

  File file = LittleFS.open("/rfids.json", FILE_READ);
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
  if (!this->saveList(this->allowedRFIDs))
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
  if (count > 0 && !this->saveList(this->allowedRFIDs))
  {
    Serial.println("ERRO: Falha ao salvar lista!");
    return -1;
  }

  return count;
}

bool Storage::saveList(std::vector<unsigned long> listToSave)
{
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.createNestedArray("rfids");

  for (unsigned long rfid : listToSave)
  {
    array.add(rfid);
  }

  File file = LittleFS.open("/rfids.json", FILE_WRITE);
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

void Storage::saveAccessHistory(const std::vector<unsigned long> &accessHistory)
{
  int address = ACCESS_HISTORY_START;

  // Save the count of access entries (limit to MAX_ACCESS_ENTRIES)
  byte count = (accessHistory.size() > MAX_ACCESS_ENTRIES) ? MAX_ACCESS_ENTRIES : accessHistory.size();
  EEPROM.write(address, count);
  address += sizeof(byte);

  // Save each access entry
  for (size_t i = 0; i < count; i++)
  {
    EEPROM.put(address, accessHistory[i]);
    address += sizeof(unsigned long);
  }

#ifdef ESP8266
  EEPROM.commit();
#elif defined(ESP32)
  EEPROM.commit();
#endif
  Serial.print("Saved ");
  Serial.print(count);
  Serial.println(" entries to access history in EEPROM");
}

std::vector<unsigned long> Storage::loadAccessHistory()
{
  std::vector<unsigned long> accessHistory;
  int address = ACCESS_HISTORY_START;

  // Read the count of access entries
  byte count = EEPROM.read(address);
  address += sizeof(byte);

  // Validate count to prevent corruption issues
  if (count > MAX_ACCESS_ENTRIES)
  {
    Serial.print("Invalid access history count in EEPROM: ");
    Serial.println(count);
    return accessHistory;
  }

  // Read each access entry
  for (byte i = 0; i < count; i++)
  {
    unsigned long cardId;
    EEPROM.get(address, cardId);
    address += sizeof(unsigned long);

    // Basic validation - most cards should have non-zero IDs
    // We'll also skip very large values that might indicate corruption
    if (cardId != 0 && cardId < 0xFFFFFFFF)
    {
      accessHistory.push_back(cardId);
    }
  }

  Serial.print("Loaded ");
  Serial.print(accessHistory.size());
  Serial.println(" access entries from EEPROM");

  return accessHistory;
}

std::vector<unsigned long> Storage::getAll()
{
  std::vector<unsigned long> rfids;

  File file = LittleFS.open("/rfids.json", FILE_READ);
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