#include "RFIDModule.h"

RFIDModule::RFIDModule(MFRC522 *reader, Storage *storage, Actuator *actuator)
    : reader(reader), storage(storage), actuator(actuator) {}

void RFIDModule::begin()
{
  reader->PCD_Init();
  // Load access history on startup
  accessedCards = storage->loadAccessHistory();
  if (!accessedCards.empty())
  {
    lastAccessedCardId = accessedCards.back();
    Serial.print("Loaded access history with ");
    Serial.print(accessedCards.size());
    Serial.println(" entries");
  }
}

void RFIDModule::loop()
{
  if (reader->PICC_IsNewCardPresent() && reader->PICC_ReadCardSerial())
  {
    checkCard();
    reader->PICC_HaltA();
  }
}

void RFIDModule::checkCard()
{
  unsigned long cardId = convertUID(reader->uid.uidByte, reader->uid.size);
  bool allowed = storage->isAllowed(cardId);
  if (allowed)
  {
    addToAccessHistory(cardId);
    lastAccessedCardId = cardId;
    actuator->open();
  }

  if (accessCallback)
    accessCallback(allowed, cardId);
}

void RFIDModule::addToAccessHistory(unsigned long cardId)
{
  accessedCards.push_back(cardId);

  if (accessedCards.size() > MAX_ACCESS_HISTORY)
  {
    accessedCards.erase(accessedCards.begin());
  }

  storage->saveAccessHistory(accessedCards);
  Serial.print("Access history size: ");
  Serial.println(accessedCards.size());
}

std::vector<unsigned long> RFIDModule::getLastAccesses() const
{
  return accessedCards;
}

unsigned long RFIDModule::getLastAccessedCardId() const
{
  return lastAccessedCardId;
}

void RFIDModule::clearAccessHistory(JsonDocument &doc, String &response)
{
  String client = doc["client"] | "";
  String command = doc["command"] | "get_access_history";
  StaticJsonDocument<256> respDoc;
  respDoc["callBack"]["client"] = client;
  respDoc["callBack"]["command"] = command;
  respDoc["callBack"]["status"] = "success";
  respDoc["callBack"]["message"] = "Access history cleared";
  serializeJson(respDoc, response);
  
  accessedCards.clear();
  lastAccessedCardId = 0;
  // Save empty history to persistent storage
  storage->saveAccessHistory(accessedCards);
  Serial.println("Access history cleared");
}

void RFIDModule::setAccessCallback(void (*callback)(bool success, unsigned long cardId))
{
  accessCallback = callback;
}

unsigned long RFIDModule::convertUID(uint8_t *uid, byte size)
{
  unsigned long result = 0;
  for (int i = 0; i < size; i++)
  {
    result |= ((unsigned long)uid[i]) << (8 * i);
  }
  return result;
}