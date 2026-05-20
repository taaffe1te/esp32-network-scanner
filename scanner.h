#pragma once

#include <WiFi.h>
#include "config.h"
#include "icmp_ping.h"


struct Device {
  char    ip[16];
  uint8_t mac[6];
  bool    hasMac;
  bool    authorized;
  bool    isNew;
};


struct ScanResult {
  Device  devices[MAX_DEVICES];
  uint8_t count;
};


class NetworkScanner {
public:

  ScanResult scan(IPAddress localIP, IPAddress mask) {
    ScanResult result;
    result.count = 0;

    uint32_t ipInt   = toUint32(localIP);
    uint32_t maskInt = toUint32(mask);
    uint32_t base    = ipInt & maskInt;
    uint32_t hostMin = base + 1;
    uint32_t hostMax = (base | ~maskInt) - 1;


    if ((hostMax - hostMin) > 253) hostMax = hostMin + 253;

    Serial.printf("[Скан] Диапазон: %d.%d.%d.1 — .%d\n",
      (hostMin>>24)&0xFF,(hostMin>>16)&0xFF,(hostMin>>8)&0xFF,
      hostMax & 0xFF);

    for (uint32_t ip = hostMin;
         ip <= hostMax && result.count < MAX_DEVICES; ip++) {

      if (ip == ipInt) continue;

      PingResult pr = _pinger.ping(ip, PING_TIMEOUT_MS);

      if (pr.alive) {
        Device &d = result.devices[result.count];
        uint32ToStr(ip, d.ip);
        memcpy(d.mac, pr.mac, 6);
        d.hasMac     = pr.hasMac;
        d.authorized = false;
        d.isNew      = false;
        result.count++;

        Serial.printf("[+] %s  MAC: %02X:%02X:%02X:%02X:%02X:%02X%s\n",
          d.ip,
          d.mac[0], d.mac[1], d.mac[2],
          d.mac[3], d.mac[4], d.mac[5],
          d.hasMac ? "" : " (нет MAC)");
      }
    }

    Serial.printf("[Скан] Завершено. Найдено: %d\n", result.count);
    return result;
  }

private:
  IcmpPing _pinger;


  void uint32ToStr(uint32_t ip, char *buf) {
    sprintf(buf, "%d.%d.%d.%d",
      (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF, ip&0xFF);
  }

  uint32_t toUint32(IPAddress ip) {
    return ((uint32_t)ip[0]<<24)|((uint32_t)ip[1]<<16)|
           ((uint32_t)ip[2]<<8)| (uint32_t)ip[3];
  }
};
