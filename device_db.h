#pragma once

#include <Preferences.h>
#include "scanner.h"
#include "config.h"

static void macToKey(const uint8_t mac[6], char *buf) {
  sprintf(buf, "%02X%02X%02X%02X%02X%02X",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

static void macToStr(const uint8_t mac[6], char *buf) {
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}


class DeviceDB {
public:


  void load(Preferences &prefs) {
    _firstRun = !prefs.getBool("init", false);
    Serial.printf("[DB] firstRun=%s\n", _firstRun?"да":"нет");
  }


  bool update(Device *found, uint8_t count, Preferences &prefs) {
    _total = (count < MAX_DEVICES) ? count : MAX_DEVICES;
    _authCount = _unknownCount = 0;

    for (uint8_t i = 0; i < _total; i++)
      _devices[i] = found[i];

    if (_firstRun) {

      for (uint8_t i = 0; i < _total; i++) {
        _devices[i].authorized = true;
        _devices[i].isNew      = false;
        if (_devices[i].hasMac) {
          char key[14];
          macToKey(_devices[i].mac, key);
          prefs.putBool(key, true);
        }
      }
      prefs.putBool("init", true);
      _firstRun  = false;
      _authCount = _total;
      Serial.printf("[DB] Первый запуск: авторизовано %d устройств\n", _total);
      return false;
    }


    for (uint8_t i = 0; i < _total; i++) {
      bool known = false;
      if (_devices[i].hasMac) {
        char key[14];
        macToKey(_devices[i].mac, key);
        known = prefs.getBool(key, false);
      } else {

        known = true;
      }
      _devices[i].authorized = known;
      _devices[i].isNew      = !known;
      known ? _authCount++ : _unknownCount++;
    }

    return (_unknownCount > 0);
  }


  void authorize(uint8_t idx, Preferences &prefs) {
    if (idx >= _total || !_devices[idx].hasMac) return;
    char key[14];
    macToKey(_devices[idx].mac, key);
    prefs.putBool(key, true);
    _devices[idx].authorized = true;
    _devices[idx].isNew      = false;
    if (_unknownCount > 0) _unknownCount--;
    _authCount++;
  }


  void reset(Preferences &prefs) {
    prefs.clear();
    _firstRun = _total = _authCount = _unknownCount = 0;
  }


  uint8_t totalCount()      const { return _total; }
  uint8_t authorizedCount() const { return _authCount; }
  uint8_t unknownCount()    const { return _unknownCount; }
  Device* getDevices()            { return _devices; }


  uint8_t getUnknown(Device *buf, uint8_t bufSize) const {
    uint8_t n = 0;
    for (uint8_t i = 0; i < _total && n < bufSize; i++)
      if (!_devices[i].authorized) buf[n++] = _devices[i];
    return n;
  }

private:
  Device  _devices[MAX_DEVICES];
  uint8_t _total        = 0;
  uint8_t _authCount    = 0;
  uint8_t _unknownCount = 0;
  bool    _firstRun     = true;
};
