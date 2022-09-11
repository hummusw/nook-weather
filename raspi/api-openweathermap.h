#ifndef NOOK_WEATHER_API_OPENWEATHERMAP_H
#define NOOK_WEATHER_API_OPENWEATHERMAP_H

#include <nlohmann/json.hpp>

#include "api-base.h"

class OpenWeatherMap : API {
public:
    explicit OpenWeatherMap(double lat, double lon, const std::string & appid);
    CurrentWeather get_current() override;
    Precipitation get_precipitation() override;
    std::vector<HourlyWeather> get_hourly(int hours) override;
    std::vector<DailyWeather> get_daily(int days) override;
    std::vector<WeatherAlert> get_alerts() override;
private:
    nlohmann::json response_onecall;
    nlohmann::json response_airpollution;
    static size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s);
    AQI get_airquality();
};

#endif //NOOK_WEATHER_API_OPENWEATHERMAP_H
