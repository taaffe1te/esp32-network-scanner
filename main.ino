#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "config.h"
#include "st7789.h"
#include "icmp_ping.h"
#include "scanner.h"
#include "device_db.h"
#include "display.h"


#define BTN_NEXT  35
#define BTN_PREV   0


Preferences    prefs;
DisplayManager display;
NetworkScanner scanner;
DeviceDB       db;
WiFiManager    wifiManager;


bool     wifiOk        = false;
bool     threat        = false;
uint8_t  screen        = 0;
uint32_t lastScanMs    = 0;
uint32_t scanCount     = 0;
uint32_t bootMs        = 0;


bool     btn1Prev      = HIGH;
bool     btn2Prev      = HIGH;
uint32_t btn2DownMs    = 0;
bool     btn2LongDone  = false;


Device  unknownBuf[MAX_DEVICES];
uint8_t unknownCount = 0;


void setup() {
  Serial.begin(115200);
  bootMs = millis();

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);


  display.begin();
  display.showBoot("Starting...");
  delay(500);


  prefs.begin("netmon", false);
  db.load(prefs);


  if (digitalRead(BTN_PREV) == LOW) {
    display.showBoot("Resetting WiFi...");
    delay(500);


    uint32_t holdStart = millis();
    while (digitalRead(BTN_PREV) == LOW) {
      if (millis() - holdStart > 3000) {
        wifiManager.resetSettings();
        display.showBoot("WiFi reset! Restarting...");
        delay(1500);
        ESP.restart();
      }
    }
  }


  setupWiFi();


  if (wifiOk) {
    display.showBoot("First scan...");
    delay(300);
    doScan();
  }
}


void loop() {
  handleButtons();

  if (wifiOk && (millis() - lastScanMs >= SCAN_INTERVAL_MS))
    doScan();


  if (WiFi.status() != WL_CONNECTED && wifiOk) {
    wifiOk = false;
    display.showBoot("Reconnecting...");
    WiFi.reconnect();
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
      delay(500);
    }
    wifiOk = (WiFi.status() == WL_CONNECTED);
  }


  static uint32_t lastDraw = 0;
  if (millis() - lastDraw >= 2000) {
    lastDraw = millis();
    drawScreen();
  }

  delay(10);
}

void setupWiFi() {
  wifiManager.setTitle("NetMonitor Setup");
  wifiManager.setConfigPortalTimeout(180);


  wifiManager.setAPCallback([](WiFiManager *wm) {
    display.showBoot("Connect to:");

    delay(100);
    display.showSetupMode(wm->getConfigPortalSSID().c_str());
  });


  wifiManager.setSaveConfigCallback([]() {
    display.showBoot("Saved! Connecting...");
  });

  display.showBoot("Connecting WiFi...");


  bool connected = wifiManager.autoConnect("NetMonitor-Setup", "12345678");

  if (connected) {
    wifiOk = true;
    Serial.printf("[WiFi] Подключён: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    wifiOk = false;
    Serial.println("[WiFi] Не удалось подключиться");
    display.showBoot("WiFi failed!");
    delay(2000);
  }
}


void doScan() {
  display.showScanProgress(0);

  ScanResult res = scanner.scan(WiFi.localIP(), WiFi.subnetMask());
  scanCount++;
  lastScanMs = millis();

  threat = db.update(res.devices, res.count, prefs);
  unknownCount = db.getUnknown(unknownBuf, MAX_DEVICES);

  Serial.printf("[Скан] Устройств: %d, угроз: %d\n",
                res.count, unknownCount);
}


void handleButtons() {
  bool b1 = digitalRead(BTN_NEXT);
  bool b2 = digitalRead(BTN_PREV);


  if (btn1Prev == LOW && b1 == HIGH)
    screen = (screen + 1) % SCREEN_COUNT;
  btn1Prev = b1;


  if (b2 == LOW && btn2Prev == HIGH) {
    btn2DownMs   = millis();
    btn2LongDone = false;
  }
  if (b2 == LOW && !btn2LongDone) {
    if (millis() - btn2DownMs >= LONG_PRESS_MS) {
      btn2LongDone = true;
      if (wifiOk) doScan();
    }
  }
  if (btn2Prev == LOW && b2 == HIGH && !btn2LongDone)
    screen = (screen + SCREEN_COUNT - 1) % SCREEN_COUNT;

  btn2Prev = b2;
}


void drawScreen() {
  uint32_t uptime      = (millis() - bootMs) / 1000;
  uint32_t sinceLastSc = (millis() - lastScanMs) / 1000;

  switch (screen) {
    case 0:
      display.drawStatusScreen(
        wifiOk,
        WiFi.SSID().c_str(),
        wifiOk ? WiFi.localIP().toString().c_str() : "---",
        WiFi.RSSI(),
        threat
      );
      break;
    case 1:
      display.drawDevicesScreen(
        db.totalCount(),
        db.authorizedCount(),
        db.unknownCount(),
        db.getDevices()
      );
      break;
    case 2:
      display.drawThreatsScreen(unknownBuf, unknownCount);
      break;
    case 3:
      display.drawStatsScreen(sinceLastSc, uptime, scanCount);
      break;
  }
}
