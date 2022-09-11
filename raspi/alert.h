#ifndef NOOK_WEATHER_ALERT_H
#define NOOK_WEATHER_ALERT_H

#include <string>

class WeatherAlert {
public:
    explicit WeatherAlert(std::string name, int64_t start, int64_t end);
    std::string get_name() const;               // Getter method for name
    std::string get_time() const;               // Gets time description
private:
    std::string name;                           // Name of alert
    int64_t start;                              // Units: Unix time
    int64_t end;                                // Units: Unix time
};

#endif //NOOK_WEATHER_ALERT_H
