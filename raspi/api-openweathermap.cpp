#include <sstream>
#include <algorithm>

#include <curl/curl.h>

#include "api-openweathermap.h"

/**
 * Writes data from curl's response to a string
 *
 * @param [in] contents data from curl
 * @param [in] size size of each element (always 1)
 * @param [in] nmemb number of elements/characters
 * @param [in,out] s string to save data to
 * @return number of elements handled correctly (returns 0 if error)
 */
size_t OpenWeatherMap::curl_callback(void *contents, size_t size, size_t nmemb, std::string *s) {
    size_t newLength = size*nmemb;
    try {
        s->append((char *) contents, nmemb);
    } catch (std::bad_alloc &e) {
        return 0;
    } catch (std::length_error &e) {
        return 0;
    }
    return newLength;
}



/**
 * Intializes OpenWeatherMap object and gets data from server
 *
 * @param [in] lat Latitude of location
 * @param [in] lon Longitude of location
 * @param [in] appid key to use for the API call
 */
OpenWeatherMap::OpenWeatherMap(const double lat, const double lon, const std::string & appid){
    // Initialize variables
    std::stringstream onecall_urlstream = std::stringstream();
    onecall_urlstream << "https://api.openweathermap.org/data/3.0/onecall"
                         "?lat=" << lat << "&lon=" << lon << "&exclude=minutely&units=metric&appid=" << appid;
    std::string response_onecall_str;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Unable to initialize curl");
    }

    // Setup curl options
    curl_easy_setopt(curl, CURLOPT_URL, onecall_urlstream.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OpenWeatherMap::curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_onecall_str);

    // Perform curl operation and clean up
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Unable to get weather data");
    }
    curl_easy_cleanup(curl);

    // Parse string response as JSON and save
    response_onecall = nlohmann::json::parse(response_onecall_str);

    // Repeat to get air quality data
    std::stringstream airpollution_urlstream = std::stringstream();
    airpollution_urlstream << "http://api.openweathermap.org/data/2.5/air_pollution"
                              "?lat=" << lat << "&lon=" << lon << "&appid=" << appid;
    std::string response_airpollution_str;

    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Unable to initialize curl");
    }

    curl_easy_setopt(curl, CURLOPT_URL, airpollution_urlstream.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OpenWeatherMap::curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_airpollution_str);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Unable to get weather data");
    }
    curl_easy_cleanup(curl);

    response_airpollution = nlohmann::json::parse(response_airpollution_str);
}

/**
 * Extract and parse data from airpollution response
 *
 * Air quality indices vary greatly, so parsing has to be done based on each API's reporting format
 * OpenWeatherMap's AQI is based off of CAQI
 * @return AQI object
 */
AQI OpenWeatherMap::get_airquality() {
    // Get number and category description
    int aq_index = response_airpollution["list"][0]["main"]["aqi"];
    std::string aq_categories[] = {"Good", "Fair", "Moderate", "Poor", "Low"};
    if (aq_index > 5) {
        aq_index = 5;
    }
    if (aq_index < 1) {
        aq_index = 1;
    }
    std::string aq_category = aq_categories[aq_index - 1];

    // If air quality is good, don't bother with calculating pollutant levels
    if (aq_index == 1) {
        return AQI(aq_index, aq_category);
    }

    // Get pollutants
    std::string pollutants[] = {"no2", "pm10", "o3", "pm2.5"};

    // Get pollutant concentrations
    double no2 = response_airpollution["list"][0]["components"]["no2"];
    double pm10 = response_airpollution["list"][0]["components"]["pm10"];
    double o3 = response_airpollution["list"][0]["components"]["o3"];
    double pm25 = response_airpollution["list"][0]["components"]["pm2_5"];
    double concentrations[4] = {no2, pm10, o3, pm25};

    // Use CAQI scales
    double no2_cutoffs[5] = {0, 50, 100, 200, 400};
    double pm10_cutoffs[5] = {0, 25, 50, 90, 180};
    double o3_cutoffs[5] = {0, 60, 120, 180, 240};
    double pm25_cutoffs[5] = {0, 15, 30, 55, 110};
    double *cutoffs[4] = {no2_cutoffs, pm10_cutoffs, o3_cutoffs, pm25_cutoffs};

    // Calculate most severe pollutant
    double severity[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        double concentration = concentrations[i];

        // If concentration falls within range
        for (int index = 1; index < 5; index++) {
            if (concentration <= cutoffs[i][index]) {
                severity[i] = index + (concentration - cutoffs[i][index]) / (cutoffs[i][index] - cutoffs[i][index - 1]);
                break;
            }
        }

        // If concentration is beyond range, extrapolate
        if (concentration > cutoffs[i][4]) {
            severity[i] = concentration / cutoffs[i][4] * 5;
        }
    }

    // Find maximum
    int max_i = 0;
    double max_severity = severity[0];
    for (int i = 1; i < 4; i++) {
        if (severity[i] > max_severity) {
            max_i = i;
            max_severity = severity[i];
        }
    }

    // Return AQI object
    return AQI(aq_index, aq_category, pollutants[max_i]);
}

/**
 * Gets current weather data from response
 *
 * @returns CurrentWeather struct with data from earlier response
 */
CurrentWeather OpenWeatherMap::get_current() {
    // Extract data from onecall response
    int64_t timestamp = response_onecall["current"]["dt"];
    double temp = response_onecall["current"]["temp"];
    double feels_like = response_onecall["current"]["feels_like"];
    std::string weather = response_onecall["current"]["weather"][0]["description"];
    std::string icon = response_onecall["current"]["weather"][0]["icon"];
    Beaufort wind = Beaufort((double) response_onecall["current"]["wind_speed"]);
    UVIndex uvi = UVIndex(response_onecall["current"]["uvi"]);
    double humidity = (double) response_onecall["current"]["humidity"] / 100;

    // Extract air quality from airpollution response
    AQI aqi = get_airquality();

    // Create and return struct
    return CurrentWeather{timestamp, temp, feels_like, weather, icon, aqi, wind, uvi, humidity};
}

/**
 * Gets precipitation chances from response
 *
 * @returns Precipitation struct with data from response
 */
Precipitation OpenWeatherMap::get_precipitation() {
    // Extract data from response
    double hour = response_onecall["hourly"][0]["pop"];
    double today = response_onecall["daily"][0]["pop"];

    // Create and return struct
    return Precipitation{hour, today};
}

/**
 * Gets hourly weather data from response
 * Check size of returned vector for how many hours were extracted successfully, might not be the same as input parameter
 *
 * @param [in] hours number of hours to get
 * @returns vector of HourlyWeather structs from response
 */
std::vector<HourlyWeather> OpenWeatherMap::get_hourly(const int hours) {
    // Determine maximum number of hours available in response
    int extractable_hours = std::min(hours, (int) response_onecall["hourly"].size());

    // Handle zero/negative number of hours
    if (extractable_hours <= 0) {
        return std::vector<HourlyWeather>(0);
    }

    // Create and populate vector
    std::vector<HourlyWeather> hourly(extractable_hours);
    for (int i = 0; i < extractable_hours; i++) {
        hourly[i].timestamp = response_onecall["hourly"][i]["dt"];
        hourly[i].temp = response_onecall["hourly"][i]["temp"];
        hourly[i].pop = response_onecall["hourly"][i]["pop"];
        hourly[i].icon = response_onecall["hourly"][i]["weather"][0]["icon"];
    }

    return hourly;
}

/**
 * Gets daily weather data from response
 * Check size of returned vector for how many days were extracted successfully, might not be the same as input parameter
 *
 * @param [in] days number of days to get
 * @returns vector of DailyWeather structs from response
 */
std::vector<DailyWeather> OpenWeatherMap::get_daily(const int days) {
    // Determine maximum number of hours available in response
    int extractable_days = std::min(days, (int) response_onecall["daily"].size());

    // Handle zero/negative number of hours
    if (extractable_days <= 0) {
        return std::vector<DailyWeather>(0);
    }

    // Create and populate vector
    std::vector<DailyWeather> daily(extractable_days);
    for (int i = 0; i < extractable_days; i++) {
        daily[i].timestamp = response_onecall["daily"][i]["dt"];
        daily[i].weather = response_onecall["daily"][i]["weather"][0]["description"];
        daily[i].icon = response_onecall["daily"][i]["weather"][0]["icon"];
        daily[i].hi = response_onecall["daily"][i]["temp"]["max"];

        // usually this would be the lowest temp between today and tomorrow
        // since this usually happens in the early hours of the next day, just use the next day's min temperature
        if (response_onecall["daily"].size() > i + 1) {
            daily[i].lo = response_onecall["daily"][i+1]["temp"]["min"];
        } else {
            daily[i].lo = NAN;
        }
    }

    return daily;
}

/**
 * Gets weather alerts from response
 *
 * @returns vector of Alert objects from response
 */
std::vector<WeatherAlert> OpenWeatherMap::get_alerts() {
    // Determine number of current weather alerts
    unsigned long num_alerts = response_onecall["alerts"].size();

    // Handle zero alerts
    if (num_alerts <= 0) {
        return std::vector<WeatherAlert>();
    }

    // Create and populate vector
    std::vector<WeatherAlert> alerts;
    for (int i = 0; i < num_alerts; i++) {
        std::string name = response_onecall["alerts"][i]["event"];
        int64_t start = response_onecall["alerts"][i]["start"];
        int64_t end = response_onecall["alerts"][i]["end"];

        alerts.emplace_back(WeatherAlert(name, start, end));
    }

    return alerts;
}
