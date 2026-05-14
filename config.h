#ifndef CONFIG_H
#define CONFIG_H

// پین‌ها
#define SDA_PIN 4        // GPIO4 (D2)
#define SCL_PIN 5        // GPIO5 (D1)
#define RELAY_PIN 12     // GPIO12 (D6)
#define BUTTON_PIN 0     // GPIO0 (D3)

// تنظیمات AP
#define AP_SSID "AstroSwitch"
#define AP_PASS "12345678"
#define AP_TIMEOUT 300000  // 5 دقیقه

// آدرس EEPROM
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xA5C3

// حالت‌های کاری
enum OperationMode {
  MODE_SIMPLE_ASTRO = 0,      // طلوع تا غروب
  MODE_ASTRO_EARLY_OFF = 1,   // غروب + مدت مشخص
  MODE_SIMPLE_TIMER = 2,      // تایمر ساده (5 بازه)
  MODE_WEEKLY_TIMER = 3,      // تایمر هفتگی
  MODE_PULSE = 4,             // پالس
  MODE_COUNTDOWN = 5,         // شمارش معکوس
  MODE_MANUAL = 6,            // دستی
  MODE_VACATION = 7           // تعطیلات (همیشه خاموش)
};

// ساختار تایمر ساده
struct SimpleTimer {
  uint8_t onHour;
  uint8_t onMinute;
  uint8_t offHour;
  uint8_t offMinute;
  bool enabled;
};

// ساختار تایمر هفتگی
struct WeeklyTimer {
  SimpleTimer slots[3];  // 3 بازه در روز
};

// ساختار تنظیمات
struct Settings {
  uint16_t magic;                    // شناسه اعتبارسنجی
  float latitude;                    // عرض جغرافیایی
  float longitude;                   // طول جغرافیایی
  int8_t timezone;                   // منطقه زمانی
  OperationMode mode;                // حالت فعال
  
  // حالت 1: Astro Early Off
  uint16_t earlyOffMinutes;          // دقیقه خاموشی زودتر
  
  // حالت 2: Simple Timer
  SimpleTimer simpleTimers[5];       // 5 بازه زمانی
  
  // حالت 3: Weekly Timer
  WeeklyTimer weeklyTimers[7];       // 7 روز هفته
  
  // حالت 4: Pulse
  uint16_t pulseOnSeconds;           // ثانیه روشن
  uint16_t pulseOffSeconds;          // ثانیه خاموش
  
  // حالت 5: Countdown
  uint32_t countdownSeconds;         // ثانیه شمارش معکوس
  uint32_t countdownStart;           // زمان شروع (timestamp)
  
  // حالت 6: Manual
  bool manualState;                  // وضعیت دستی
  
  // آفست‌های نجومی
  int8_t sunriseOffset;              // آفست طلوع (دقیقه)
  int8_t sunsetOffset;               // آفست غروب (دقیقه)
  
  uint16_t checksum;                 // چک‌سام
};

// مقادیر پیش‌فرض
#define DEFAULT_LAT 35.6892
#define DEFAULT_LON 51.3890
#define DEFAULT_TZ 3  // UTC+3:30 ایران

#endif
