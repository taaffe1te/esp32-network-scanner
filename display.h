#pragma once
 
#include "st7789.h"
#include "scanner.h"
 
#define SCR_W   240
#define SCR_H   135
#define HDR_H    18
#define M         4
 

class DisplayManager {
public:
 
  void begin() {
    _tft.begin();
    clear();
  }
 
  void clear() { _tft.fillScreen(COLOR_BLACK); }
 

  void showBoot(const char *msg) {
    _tft.fillScreen(COLOR_BLACK);
    _tft.drawString(M, 18, "NetMonitor v1.0", COLOR_CYAN, COLOR_BLACK, 2);
    _tft.drawString(M, 52, "ESP32 T-Display",  COLOR_GREY, COLOR_BLACK, 1);
    _tft.drawHLine(M, 68, SCR_W - M*2, COLOR_DKGREY);
    _tft.drawString(M, 78, msg, COLOR_WHITE, COLOR_BLACK, 1);
  }

  void showSetupMode(const char *apName) {
    _tft.fillScreen(COLOR_BLACK);
    drawHeader("WIFI SETUP", COLOR_DKBLUE);

    int y = HDR_H + 8;
    _tft.drawString(M, y, "1. Connect phone to:", COLOR_WHITE, COLOR_BLACK, 1);
    y += 14;
    _tft.drawString(M, y, apName, COLOR_CYAN, COLOR_BLACK, 1);
    y += 16;

    _tft.drawString(M, y, "2. Password:", COLOR_WHITE, COLOR_BLACK, 1);
    y += 14;
    _tft.drawString(M, y, "12345678", COLOR_CYAN, COLOR_BLACK, 1);
    y += 16;

    _tft.drawString(M, y, "3. Open browser:", COLOR_WHITE, COLOR_BLACK, 1);
    y += 14;
    _tft.drawString(M, y, "192.168.4.1", COLOR_GREEN, COLOR_BLACK, 1);
  }


  void showScanProgress(uint8_t percent) {
    static bool init = false;
    if (!init) {
      _tft.fillScreen(COLOR_BLACK);
      drawHeader("SCANNING NETWORK", COLOR_DKBLUE);
      _tft.drawString(M, 36, "Pinging hosts...", COLOR_WHITE, COLOR_BLACK, 1);
      _tft.drawRect(M, 80, SCR_W - M*2, 14, COLOR_GREY);
      init = true;
    }
    uint16_t barW = ((uint32_t)(SCR_W - M*2 - 4) * percent) / 100;
    _tft.fillRect(M + 2, 82, barW, 10, COLOR_CYAN);
    char buf[8]; sprintf(buf, "%d%%", percent);
    _tft.drawString(M, 100, buf, COLOR_GREY, COLOR_BLACK, 1);
    if (percent == 0) init = false;
  }
 

  void drawStatusScreen(bool connected, const char *ssid,
                        const char *ip, int rssi, bool threat) {
    _tft.fillRect(0, HDR_H, SCR_W, SCR_H - HDR_H, COLOR_BLACK);
    uint16_t hdrColor = threat ? COLOR_RED : COLOR_DKBLUE;
    drawHeader(threat ? "! THREAT DETECTED !" : "NETWORK STATUS", hdrColor);
 
    int y = HDR_H + 5;
 

    label(M, y, "Wi-Fi:");
    _tft.drawString(50, y, connected ? "CONNECTED" : "OFFLINE",
                    connected ? COLOR_GREEN : COLOR_RED, COLOR_BLACK, 1);
    y += 15;
 

    label(M, y, "Net:");
    _tft.drawString(50, y, ssid, COLOR_WHITE, COLOR_BLACK, 1);
    y += 15;
 

    label(M, y, "IP:");
    _tft.drawString(50, y, ip, COLOR_CYAN, COLOR_BLACK, 1);
    y += 15;
 

    label(M, y, "Signal:");
    char rssiStr[12]; sprintf(rssiStr, "%d dBm", rssi);
    _tft.drawString(55, y, rssiStr, rssiColor(rssi), COLOR_BLACK, 1);
    drawSignalBars(170, y, rssi);
    y += 16;
 

    uint16_t sColor = threat ? COLOR_RED : COLOR_GREEN;
    _tft.fillRect(M, y, SCR_W - M*2, 14, sColor);
    const char *statusStr = threat ? "NEW DEVICE DETECTED"
                                   : "NETWORK IS SECURE";
    uint8_t len = strlen(statusStr);
    int16_t tx = (SCR_W - len * 6) / 2;
    _tft.drawString(tx, y + 4, statusStr, COLOR_BLACK, sColor, 1);
 
    drawNavHint();
  }
 

  void drawDevicesScreen(uint8_t total, uint8_t auth,
                         uint8_t unknown, Device *devices) {
    _tft.fillRect(0, HDR_H, SCR_W, SCR_H - HDR_H, COLOR_BLACK);
    drawHeader("DEVICES", COLOR_DKBLUE);
 
    int y = HDR_H + 4;
    char buf[40];
    sprintf(buf, "Total:%d  Auth:%d  New:%d", total, auth, unknown);
    _tft.drawString(M, y, buf,
                    unknown > 0 ? COLOR_ORANGE : COLOR_GREEN,
                    COLOR_BLACK, 1);
    y += 13;
    _tft.drawHLine(M, y, SCR_W - M*2, COLOR_DKGREY);
    y += 4;
 
    uint8_t show = total < 5 ? total : 5;
    for (uint8_t i = 0; i < show; i++) {
      uint16_t c = devices[i].authorized ? COLOR_WHITE : COLOR_ORANGE;
      const char *mark = devices[i].authorized ? "  " : "! ";
      _tft.drawString(M,      y, mark,          COLOR_ORANGE, COLOR_BLACK, 1);
      _tft.drawString(M + 14, y, devices[i].ip, c,           COLOR_BLACK, 1);
      y += 13;
    }
    if (total > 5) {
      sprintf(buf, "  ...%d more", total - 5);
      _tft.drawString(M, y, buf, COLOR_GREY, COLOR_BLACK, 1);
    }
 
    drawNavHint();
  }
 

  void drawThreatsScreen(Device *unknowns, uint8_t count) {
    _tft.fillRect(0, HDR_H, SCR_W, SCR_H - HDR_H, COLOR_BLACK);
 
    if (count == 0) {
      drawHeader("THREATS", COLOR_GREEN);
      _tft.drawString(30, 60, "No threats detected",
                      COLOR_GREEN, COLOR_BLACK, 1);
      drawNavHint();
      return;
    }
 
    drawHeader("! NEW DEVICES !", COLOR_RED);
    int y = HDR_H + 5;
 
    uint8_t show = count < 3 ? count : 3;
    for (uint8_t i = 0; i < show; i++) {
      _tft.drawString(M, y, unknowns[i].ip, COLOR_ORANGE, COLOR_BLACK, 1);
      y += 12;
      char macBuf[20];
      macToStr(unknowns[i].mac, macBuf);
      _tft.drawString(M + 6, y, macBuf, COLOR_GREY, COLOR_BLACK, 1);
      y += 14;
    }
    if (count > 3) {
      char buf[24]; sprintf(buf, "...%d more devices", count - 3);
      _tft.drawString(M, y, buf, COLOR_RED, COLOR_BLACK, 1);
    }
 
    drawNavHint();
  }
 

  void drawStatsScreen(uint32_t sinceLastScan,
                       uint32_t uptime, uint32_t scans) {
    _tft.fillRect(0, HDR_H, SCR_W, SCR_H - HDR_H, COLOR_BLACK);
    drawHeader("STATISTICS", COLOR_DKBLUE);
 
    int y = HDR_H + 8;
    char buf[32];
 
    label(M, y, "Last scan:");
    sprintf(buf, "%02d:%02d min", sinceLastScan/60, sinceLastScan%60);
    _tft.drawString(80, y, buf, COLOR_WHITE, COLOR_BLACK, 1);
    y += 18;
 
    label(M, y, "Uptime:");
    uint32_t h = uptime/3600, m=(uptime%3600)/60, s=uptime%60;
    sprintf(buf, "%02d:%02d:%02d", h, m, s);
    _tft.drawString(80, y, buf, COLOR_WHITE, COLOR_BLACK, 1);
    y += 18;
 
    label(M, y, "Scans done:");
    sprintf(buf, "%d", scans);
    _tft.drawString(80, y, buf, COLOR_CYAN, COLOR_BLACK, 1);
 
    drawNavHint();
  }
 
private:
  ST7789 _tft;
 
  void drawHeader(const char *title, uint16_t color) {
    _tft.fillRect(0, 0, SCR_W, HDR_H, color);
    uint8_t len = strlen(title);
    int16_t tx  = (SCR_W - len * 6) / 2;
    _tft.drawString(tx, 5, title, COLOR_WHITE, color, 1);
  }
 
  void label(int16_t x, int16_t y, const char *text) {
    _tft.drawString(x, y, text, COLOR_GREY, COLOR_BLACK, 1);
  }
 
  void drawNavHint() {
    _tft.drawString(M, SCR_H - 9,
      "B1:next B2:prev B2(2s):scan",
      COLOR_DKGREY, COLOR_BLACK, 1);
  }
 
  uint16_t rssiColor(int rssi) {
    if (rssi >= -60) return COLOR_GREEN;
    if (rssi >= -75) return COLOR_ORANGE;
    return COLOR_RED;
  }
 
  void drawSignalBars(int16_t x, int16_t y, int rssi) {
    uint8_t bars = 0;
    if (rssi >= -90) bars = 1;
    if (rssi >= -75) bars = 2;
    if (rssi >= -60) bars = 3;
    if (rssi >= -50) bars = 4;
    for (uint8_t i = 0; i < 4; i++) {
      uint16_t c  = (i < bars) ? rssiColor(rssi) : COLOR_DKGREY;
      int16_t  bh = 4 + i * 3;
      _tft.fillRect(x + i * 7, y + (12 - bh), 5, bh, c);
    }
  }
 
  void macToStr(const uint8_t mac[6], char *buf) {
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  }
};
