#ifndef NOOK_WEATHER_API_BASE_H
#define NOOK_WEATHER_API_BASE_H

#include <vector>

#include "weathertypes.h"

class API {
public:
    virtual CurrentWeather get_current() = 0;
    virtual Precipitation get_precipitation() = 0;
    virtual std::vector<HourlyWeather> get_hourly(int hours) = 0;
    virtual std::vector<DailyWeather> get_daily(int days) = 0;
    virtual std::vector<WeatherAlert> get_alerts() = 0;
};

#endif