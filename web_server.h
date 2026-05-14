#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESP8266WebServer.h>
#include <RTClib.h>
#include "eeprom_manager.h"
#include "relay_controller.h"

class WebServerManager {
private:
  ESP8266WebServer* server;
  EEPROMManager* eeprom;
  RTC_DS3231* rtc;
  RelayController* relay;
  
public:
  void begin(ESP8266WebServer* srv, EEPROMManager* eep, RTC_DS3231* r, RelayController* rel) {
    server = srv;
    eeprom = eep;
    rtc = r;
    relay = rel;
    
    // مسیرها
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server->on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server->on("/api/time", HTTP_GET, [this]() { handleGetTime(); });
    server->on("/api/time", HTTP_POST, [this]() { handleSetTime(); });
    server->on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server->on("/api/relay", HTTP_POST, [this]() { handleRelayControl(); });
    
    server->onNotFound([this]() { handleNotFound(); });
    
    server->begin();
    Serial.println(F("Web server started."));
  }
  
private:
  void handleRoot() {
    if (!SPIFFS.exists("/index.html")) {
      server->send(200, "text/html", getEmbeddedHTML());
      return;
    }
    
    File file = SPIFFS.open("/index.html", "r");
    server->streamFile(file, "text/html");
    file.close();
  }
  
  void handleGetSettings() {
    Settings settings = eeprom->getSettings();
    
    String json = "{";
    json += "\"latitude\":" + String(settings.latitude, 6) + ",";
    json += "\"longitude\":" + String(settings.longitude, 6) + ",";
    json += "\"timezone\":" + String(settings.timezone) + ",";
    json += "\"mode\":" + String(settings.mode) + ",";
    json += "\"sunriseOffset\":" + String(settings.sunriseOffset) + ",";
    json += "\"sunsetOffset\":" + String(settings.sunsetOffset) + ",";
    json += "\"earlyOffMinutes\":" + String(settings.earlyOffMinutes) + ",";
    json += "\"manualState\":" + String(settings.manualState ? "true" : "false");
    json += "}";
    
    server->send(200, "application/json", json);
  }
  
  void handleSaveSettings() {
    if (!server->hasArg("plain")) {
      server->send(400, "text/plain", "Bad Request");
      return;
    }
    
    Settings settings = eeprom->getSettings();
    
    // پارس JSON (ساده)
    String body = server->arg("plain");
    
    if (body.indexOf("\"latitude\":") >= 0) {
      int idx = body.indexOf("\"latitude\":") + 11;
      settings.latitude = body.substring(idx, body.indexOf(",", idx)).toFloat();
    }
    
    if (body.indexOf("\"longitude\":") >= 0) {
      int idx = body.indexOf("\"longitude\":") + 12;
      settings.longitude = body.substring(idx, body.indexOf(",", idx)).toFloat();
    }
    
    if (body.indexOf("\"mode\":") >= 0) {
      int idx = body.indexOf("\"mode\":") + 7;
      settings.mode = (OperationMode)body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    eeprom->saveSettings(settings);
    
    server->send(200, "application/json", "{\"success\":true}");
  }
  
  void handleGetTime() {
    DateTime now = rtc->now();
    
    String json = "{";
    json += "\"year\":" + String(now.year()) + ",";
    json += "\"month\":" + String(now.month()) + ",";
    json += "\"day\":" + String(now.day()) + ",";
    json += "\"hour\":" + String(now.hour()) + ",";
    json += "\"minute\":" + String(now.minute()) + ",";
    json += "\"second\":" + String(now.second());
    json += "}";
    
    server->send(200, "application/json", json);
  }
  
  void handleSetTime() {
    if (!server->hasArg("plain")) {
      server->send(400, "text/plain", "Bad Request");
      return;
    }
    
    String body = server->arg("plain");
    
    // پارس ساده
    int year = 2025, month = 1, day = 1, hour = 0, minute = 0, second = 0;
    
    if (body.indexOf("\"year\":") >= 0) {
      int idx = body.indexOf("\"year\":") + 7;
      year = body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    if (body.indexOf("\"month\":") >= 0) {
      int idx = body.indexOf("\"month\":") + 8;
      month = body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    if (body.indexOf("\"day\":") >= 0) {
      int idx = body.indexOf("\"day\":") + 6;
      day = body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    if (body.indexOf("\"hour\":") >= 0) {
      int idx = body.indexOf("\"hour\":") + 7;
      hour = body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    if (body.indexOf("\"minute\":") >= 0) {
      int idx = body.indexOf("\"minute\":") + 9;
      minute = body.substring(idx, body.indexOf(",", idx)).toInt();
    }
    
    rtc->adjust(DateTime(year, month, day, hour, minute, second));
    
    server->send(200, "application/json", "{\"success\":true}");
  }
  
  void handleStatus() {
    DateTime now = rtc->now();
    Settings settings = eeprom->getSettings();
    
    String json = "{";
    json += "\"relayState\":" + String(relay->getState() ? "true" : "false") + ",";
    json += "\"mode\":" + String(settings.mode) + ",";
    json += "\"time\":\"" + String(now.hour()) + ":" + String(now.minute()) + "\"";
    json += "}";
    
    server->send(200, "application/json", json);
  }
  
  void handleRelayControl() {
    if (!server->hasArg("plain")) {
      server->send(400, "text/plain", "Bad Request");
      return;
    }
    
    String body = server->arg("plain");
    bool state = body.indexOf("\"state\":true") >= 0;
    
    Settings settings = eeprom->getSettings();
    settings.mode = MODE_MANUAL;
    settings.manualState = state;
    eeprom->saveSettings(settings);
    
    relay->setRelay(state);
    
    server->send(200, "application/json", "{\"success\":true}");
  }
  
  void handleNotFound() {
    String path = server->uri();
    
    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      String contentType = getContentType(path);
      server->streamFile(file, contentType);
      file.close();
      return;
    }
    
    server->send(404, "text/plain", "Not Found");
  }
  
  String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
  }
  
  String getEmbeddedHTML() {
    return R"(
<!DOCTYPE html>
<html dir="rtl" lang="fa">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>کلید هوشمند نجومی</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Tahoma,Arial,sans-serif;background:#1a1a2e;color:#eee;padding:20px}
.container{max-width:600px;margin:0 auto;background:#16213e;border-radius:12px;padding:20px;box-shadow:0 4px 20px rgba(0,0,0,0.3)}
h1{text-align:center;color:#0f3460;margin-bottom:20px;font-size:24px}
.section{background:#0f3460;padding:15px;border-radius:8px;margin-bottom:15px}
.section h2{font-size: