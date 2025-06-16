#pragma once
#include <vector>
#include "SPIFFS.h"
#include <ArduinoJson.h>

class Storage
{
public:
  std::vector<unsigned long> allowedRFIDs;
  Storage();
  bool begin();
  bool loadList();
  bool isAllowed(unsigned long cardId);
  int addRFIDs(JsonDocument &doc);
  int removeRFID(unsigned long id);
  bool saveList(std::vector<unsigned long> listToSave);
  void clearList();
  std::vector<unsigned long> getAll();
};