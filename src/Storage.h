#pragma once
#include <vector>
#include "SPIFFS.h"
#include <ArduinoJson.h>

class Storage
{
public:
  Storage();
  bool begin();
  bool loadList();
  bool isAllowed(unsigned long cardId);
  int addRFIDs(JsonDocument &doc);
  int removeRFID(unsigned long id);
  bool saveList();
  void clearList();
  std::vector<unsigned long> getAll();

private:
  std::vector<unsigned long> allowedRFIDs;
};