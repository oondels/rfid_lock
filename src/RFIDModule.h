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
  std::vector<unsigned long> getLastAccesses() const;
  unsigned long getLastAccessedCardId() const;
  void clearAccessHistory();

private:
  MFRC522* reader;
  Storage* storage;
  Actuator* actuator;
  void (*accessCallback)(bool, unsigned long) = nullptr;
  std::vector<unsigned long> accessedCards;
  unsigned long lastAccessedCardId = 0;
  static const size_t MAX_ACCESS_HISTORY = 10;

  void addToAccessHistory(unsigned long cardId);
};