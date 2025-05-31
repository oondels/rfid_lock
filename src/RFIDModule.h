#pragma once
#include <MFRC522.h>
#include "Storage.h"
#include "Actuator.h"

class RFIDModule
{
public:
  RFIDModule(MFRC522* reader, Storage* storage, Actuator* actuator);
  void begin();
  void loop();
  void checkCard();
  unsigned long convertUID(uint8_t *uid, byte size);
  void setAccessCallback(void (*callback)(bool success, unsigned long cardId));

private:
  MFRC522* reader;
  Storage* storage;
  Actuator* actuator;
  void (*accessCallback)(bool, unsigned long) = nullptr;
};