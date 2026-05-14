#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <EEPROM.h>
#include "config.h"

class EEPROMManager {
public:
  void begin() {
    EEPROM.begin(EEPROM_SIZE);
    
    Settings settings;
    EEPROM.get(0, settings);
    
    // بررسی اعتبار
    if (settings.magic != EEPROM_MAGIC || !validateChecksum(settings)) {
      Serial.println(F("EEPROM invalid, loading defaults..."));
      loadDefaults();
    } else {
      Serial.println(F("EEPROM valid."));
    }
  }
  
  Settings getSettings() {
    Settings settings;
    EEPROM.get(0, settings);
    return settings;
  }
  
  void saveSettings(Settings& settings) {
    settings.magic = EEPROM_MAGIC;
    settings.checksum = calculateChecksum(settings);
    
    EEPROM.put(0, settings);
    EEPROM.commit();
    
    Serial.println(F("Settings saved to EEPROM."));
  }
  
  void loadDefaults() {
    Settings settings;
    memset(&settings, 0, sizeof(Settings));
    
    settings.magic = EEPROM_MAGIC;
    settings.latitude = DEFAULT_LAT;
    settings.longitude = DEFAULT_LON;
    settings.timezone = DEFAULT_TZ;
    settings.mode = MODE_SIMPLE_ASTRO;
    settings.sunriseOffset = 0;
    settings.sunsetOffset = 0;
    settings.earlyOffMinutes = 0;
    settings.manualState = false;
    
    saveSettings(settings);
  }
  
private:
  uint16_t calculateChecksum(Settings& settings) {
    uint16_t sum = 0;
    uint8_t* ptr = (uint8_t*)&settings;
    
    for (size_t i = 0; i < sizeof(Settings) - sizeof(uint16_t); i++) {
      sum += ptr[i];
    }
    
    return sum;
  }
  
  bool validateChecksum(Settings& settings) {
    uint16_t calculated = calculateChecksum(settings);
    return calculated == settings.checksum;
  }
};

#endif
