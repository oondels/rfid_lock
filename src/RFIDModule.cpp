#include "RFIDModule.h"

RFIDModule::RFIDModule(MFRC522 *reader, Storage *storage, Actuator *actuator)
    : reader(reader), storage(storage), actuator(actuator) {}

void RFIDModule::begin()
{
  reader->PCD_Init();
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
  
  storage->saveList(accessedCards);
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

void RFIDModule::clearAccessHistory()
{
  accessedCards.clear();
  lastAccessedCardId = 0;
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