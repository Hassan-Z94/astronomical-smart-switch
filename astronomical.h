#ifndef ASTRONOMICAL_H
#define ASTRONOMICAL_H

#include <math.h>

struct SunTimes {
  uint8_t sunriseHour;
  uint8_t sunriseMinute;
  uint8_t sunsetHour;
  uint8_t sunsetMinute;
};

class AstronomicalCalc {
public:
  SunTimes calculate(float lat, float lon, int year, int month, int day) {
    // محاسبه Julian Day
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    
    int jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    double jd = jdn + 0.5;
    
    // محاسبه زاویه روز
    double n = jd - 2451545.0;
    double L = fmod(280.460 + 0.9856474 * n, 360.0);
    double g = fmod(357.528 + 0.9856003 * n, 360.0);
    double gRad = g * M_PI / 180.0;
    
    // طول خورشیدی
    double lambda = L + 1.915 * sin(gRad) + 0.020 * sin(2 * gRad);
    double lambdaRad = lambda * M_PI / 180.0;
    
    // میل خورشید
    double epsilon = 23.439 - 0.0000004 * n;
    double epsilonRad = epsilon * M_PI / 180.0;
    double delta = asin(sin(epsilonRad) * sin(lambdaRad));
    
    // زاویه ساعتی
    double latRad = lat * M_PI / 180.0;
    double cosH = (sin(-0.833 * M_PI / 180.0) - sin(latRad) * sin(delta)) / 
                  (cos(latRad) * cos(delta));
    
    // محدود کردن به [-1, 1]
    if (cosH > 1.0) cosH = 1.0;
    if (cosH < -1.0) cosH = -1.0;
    
    double H = acos(cosH) * 180.0 / M_PI;
    
    // زمان ظهر محلی
    double eot = 4 * (L - lambda);  // معادله زمان
    double solarNoon = 720 - 4 * lon - eot;
    
    // طلوع و غروب (دقیقه از نیمه‌شب)
    double sunriseMin = solarNoon - 4 * H;
    double sunsetMin = solarNoon + 4 * H;
    
    SunTimes result;
    result.sunriseHour = ((int)sunriseMin / 60) % 24;
    result.sunriseMinute = (int)sunriseMin % 60;
    result.sunsetHour = ((int)sunsetMin / 60) % 24;
    result.sunsetMinute = (int)sunsetMin % 60;
    
    return result;
  }
};

#endif
