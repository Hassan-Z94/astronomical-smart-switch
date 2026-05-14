#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include "config.h"
#include "astronomical.h"
#include <RTClib.h>

class RelayController {
private:
  bool currentState = false;
  unsigned long pulseStartTime = 0;
  bool pulseState = false;
  
public:
  void setRelay(bool state) {
    if (state != currentState) {
      digitalWrite(RELAY_PIN, state ? HIGH : LOW);
      currentState = state;
      Serial.printf("Relay: %s\n", state ? "ON" : "OFF");
    }
  }
  
  bool getState() {
    return currentState;
  }
  
  bool shouldRelayBeOn(DateTime& now, SunTimes& sun, Settings& settings) {
    // حالت تعطیلات: همیشه خاموش
    if (settings.mode == MODE_VACATION) {
      return false;
    }
    
    // حالت دستی
    if (settings.mode == MODE_MANUAL) {
      return settings.manualState;
    }
    
    // حالت شمارش معکوس
    if (settings.mode == MODE_COUNTDOWN) {
      if (settings.countdownStart == 0) return false;
      
      uint32_t elapsed = now.unixtime() - settings.countdownStart;
      return elapsed < settings.countdownSeconds;
    }
    
    // حالت پالس
    if (settings.mode == MODE_PULSE) {
      unsigned long now_ms = millis();
      
      if (pulseStartTime == 0) {
        pulseStartTime = now_ms;
        pulseState = true;
      }
      
      unsigned long elapsed = now_ms - pulseStartTime;
      unsigned long cycleTime = (settings.pulseOnSeconds + settings.pulseOffSeconds) * 1000;
      
      if (elapsed >= cycleTime) {
        pulseStartTime = now_ms;
        elapsed = 0;
      }
      
      pulseState = elapsed < (settings.pulseOnSeconds * 1000);
      return pulseState;
    }
    
    // حالت نجومی ساده
    if (settings.mode == MODE_SIMPLE_ASTRO) {
      return isInAstroWindow(now, sun, settings);
    }
    
    // حالت نجومی با خاموشی زودتر
    if (settings.mode == MODE_ASTRO_EARLY_OFF) {
      if (!isInAstroWindow(now, sun, settings)) return false;
      
      // محاسبه زمان خاموشی
      int sunsetMinutes = sun.sunsetHour * 60 + sun.sunsetMinute + settings.sunsetOffset;
      int offMinutes = sunsetMinutes + settings.earlyOffMinutes;
      int nowMinutes = now.hour() * 60 + now.minute();
      
      return nowMinutes < offMinutes;
    }
    
    // حالت تایمر ساده
    if (settings.mode == MODE_SIMPLE_TIMER) {
      return checkSimpleTimers(now, settings.simpleTimers, 5);
    }
    
    // حالت تایمر هفتگی
    if (settings.mode == MODE_WEEKLY_TIMER) {
      int dayOfWeek = now.dayOfTheWeek();  // 0=یکشنبه
      return checkSimpleTimers(now, settings.weeklyTimers[dayOfWeek].slots, 3);
    }
    
    return false;
  }
  
private:
  bool isInAstroWindow(DateTime& now, SunTimes& sun, Settings& settings) {
    int nowMinutes = now.hour() * 60 + now.minute();
    int sunriseMinutes = sun.sunriseHour * 60 + sun.sunriseMinute + settings.sunriseOffset;
    int sunsetMinutes = sun.sunsetHour * 60 + sun.sunsetMinute + settings.sunsetOffset;
    
    // شب: از غروب تا طلوع
    if (sunsetMinutes < sunriseMinutes) {
      return nowMinutes >= sunsetMinutes || nowMinutes < sunriseMinutes;
    } else {
      return nowMinutes >= sunsetMinutes && nowMinutes < sunriseMinutes;
    }
  }
  
  bool checkSimpleTimers(DateTime& now, SimpleTimer* timers, int count) {
    int nowMinutes = now.hour() * 60 + now.minute();
    
    for (int i = 0; i < count; i++) {
      if (!timers[i].enabled) continue;
      
      int onMinutes = timers[i].onHour * 60 + timers[i].onMinute;
      int offMinutes = timers[i].offHour * 60 + timers[i].offMinute;
      
      if (onMinutes < offMinutes) {
        if (nowMinutes >= onMinutes && nowMinutes < offMinutes) return true;
      } else {
        if (nowMinutes >= onMinutes || nowMinutes < offMinutes) return true;
      }
    }
    
    return false;
  }
};

#endif
