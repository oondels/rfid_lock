#pragma once
#include <vector>
// #include "SPIFFS.h"
#include <LittleFS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

class Storage
{
public:
  static const int EEPROM_SIZE = 512;
  Storage();
  bool begin();
  bool isAllowed(unsigned long cardId);
  int addRFIDs(JsonDocument &doc);
  int removeRFID(unsigned long id);
  bool saveList(std::vector<unsigned long> listToSave);
  bool loadList();

  void saveAccessHistory(const std::vector<unsigned long> &accessedCards);
  std::vector<unsigned long> loadAccessHistory();
  std::vector<unsigned long> getAll();
  bool clearMemory();

private:
  std::vector<unsigned long> allowedRFIDs;
  std::vector<unsigned long> rfidList;
  static const int RFID_LIST_START = 0;
  static const int ACCESS_HISTORY_START = 256;
  static const int MAX_RFIDS = 50;
  static const int MAX_ACCESS_ENTRIES = 10;
};