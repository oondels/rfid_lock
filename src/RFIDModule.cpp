#include "RFIDModule.h"

RFIDModule::RFIDModule(MFRC522 *reader, Storage *storage, Actuator *actuator)
    : reader(reader), storage(storage), actuator(actuator) {}

void RFIDModule::begin()
{
  reader->PCD_Init();
}

void RFIDModule::checkCard()
{
  unsigned long cardId = convertUID(reader->uid.uidByte, reader->uid.size);
  bool allowed = storage->isAllowed(cardId);
  if (allowed)
  {
    actuator->open();
  }
  if (accessCallback)
    accessCallback(allowed, cardId);
}


void RFIDModule::loop()
{
  if (reader->PICC_IsNewCardPresent() && reader->PICC_ReadCardSerial())
  {
    checkCard();
    reader->PICC_HaltA();
  }
}

void RFIDModule::setAccessCallback(void (*callback)(bool, unsigned long))
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