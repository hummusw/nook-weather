#ifndef NOOK_WEATHER_MAIN_H
#define NOOK_WEATHER_MAIN_H

#include "beaufort.h"

struct CurrentWeather {
    int64_t timestamp;      // Units: Unix time
    double temp;            // Units: degrees Celsius
    double feels_like;      // Units: degrees Celsius
    std::string weather;    // English weather description
    std::string icon;       // Icon to use
    Beaufort wind;          // Wind info
    double humidity;        // Units: 0 (0%) - 1 (100%)
};

struct Precipitation {
    double hour;            // Units: 0 (0%) - 1 (100%)
    double today;           // Units: 0 (0%) - 1 (100%)
};

struct HourlyWeather {
    int64_t timestamp;      // Units: Unix time
    double temp;            // Units: degrees Celsius
    double pop;             // Probability of precipitation  Units: 0 (0%) - 1 (100%)
    std::string icon;       // Icon to use  (currently unused)
};

struct DailyWeather {
    int64_t timestamp;      // Units: Unix time
    double hi;              // Units: degrees Celsius
    double lo;              // Units: degrees Celsius
    std::string weather;    // English weather description
    std::string icon;       // Icon to use
};

#endif //NOOK_WEATHER_MAIN_H
