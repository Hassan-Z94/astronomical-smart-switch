#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <FS.h>

#include "config.h"
#include "eeprom_manager.h"
#include "astronomical.h"
#include "relay_controller.h"
#include "web_server.h"

RTC_DS3231 rtc;
ESP8266WebServer server(80);
EEPROMManager eepromMgr;
AstronomicalCalc astroCalc;
RelayController relayCtrl;
WebServerManager webMgr;

unsigned long lastCheck = 0;
unsigned long apStartTime = 0;
bool apMode = false;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println(F("\n=== Astronomical Smart Switch ==="));
  
  // راه‌اندازی GPIO
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // راه‌اندازی I2C و RTC
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.begin()) {
    Serial.println(F("RTC not found!"));
    while (1) delay(1000);
  }
  
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, setting time..."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // راه‌اندازی EEPROM
  eepromMgr.begin();
  
  // راه‌اندازی SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println(F("SPIFFS mount failed!"));
  }
  
  // بررسی دکمه برای ورود به حالت AP
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      enterAPMode();
    }
  }
  
  Serial.println(F("Setup complete. Running..."));
}

void loop() {
  unsigned long now = millis();
  
  // بررسی دکمه
  static unsigned long lastButtonCheck = 0;
  if (now - lastButtonCheck > 100) {
    lastButtonCheck = now;
    checkButton();
  }
  
  // بررسی حالت AP
  if (apMode) {
    server.handleClient();
    
    // خاموش کردن AP بعد از تایم‌اوت
    if (now - apStartTime > AP_TIMEOUT) {
      exitAPMode();
    }
    return;
  }
  
  // بررسی و کنترل رله هر 30 ثانیه
  if (now - lastCheck > 30000) {
    lastCheck = now;
    
    DateTime currentTime = rtc.now();
    Settings settings = eepromMgr.getSettings();
    
    // محاسبه طلوع و غروب
    SunTimes sunTimes = astroCalc.calculate(
      settings.latitude,
      settings.longitude,
      currentTime.year(),
      currentTime.month(),
      currentTime.day()
    );
    
    // تصمیم‌گیری برای وضعیت رله
    bool shouldBeOn = relayCtrl.shouldRelayBeOn(
      currentTime,
      sunTimes,
      settings
    );
    
    relayCtrl.setRelay(shouldBeOn);
    
    // لاگ وضعیت
    Serial.printf("Time: %02d:%02d | Sunrise: %02d:%02d | Sunset: %02d:%02d | Relay: %s\n",
      currentTime.hour(), currentTime.minute(),
      sunTimes.sunriseHour, sunTimes.sunriseMinute,
      sunTimes.sunsetHour, sunTimes.sunsetMinute,
      shouldBeOn ? "ON" : "OFF"
    );
  }
}

void checkButton() {
  static bool lastState = HIGH;
  static unsigned long pressStart = 0;
  
  bool currentState = digitalRead(BUTTON_PIN);
  
  if (currentState == LOW && lastState == HIGH) {
    pressStart = millis();
  }
  
  if (currentState == LOW && lastState == LOW) {
    if (millis() - pressStart > 3000 && !apMode) {
      enterAPMode();
    }
  }
  
  lastState = currentState;
}

void enterAPMode() {
  Serial.println(F("Entering AP mode..."));
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  Serial.print(F("AP IP: "));
  Serial.println(WiFi.softAPIP());
  
  webMgr.begin(&server, &eepromMgr, &rtc, &relayCtrl);
  
  apMode = true;
  apStartTime = millis();
  
  // چشمک زدن LED برای تایید
  for (int i = 0; i < 5; i++) {
    digitalWrite(RELAY_PIN, HIGH);
    delay(100);
    digitalWrite(RELAY_PIN, LOW);
    delay(100);
  }
}

void exitAPMode() {
  Serial.println(F("Exiting AP mode..."));
  
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  apMode = false;
  lastCheck = 0; // فورا بررسی کن
}
