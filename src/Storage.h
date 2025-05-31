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
  bool saveList();
  bool isAuthorized(unsigned long id);
  void addRFID(unsigned long id);
  void removeRFID(unsigned long id);
  void clearList();
  std::vector<unsigned long> getAll();

private:
  std::vector<unsigned long> allowedRFIDs;
};