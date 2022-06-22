#ifndef NOOK_WEATHER_BEAUFORT_H
#define NOOK_WEATHER_BEAUFORT_H

#include <string>

class Beaufort {
public:
    explicit Beaufort(double wind_speed);       // Construct Beaufort object from wind speed (m/s)
    double get_wind_speed() const;              // Getter method for wind_speed
    int get_number() const;                     // Getter method for number
    std::string get_description() const;        // Getter method for description
private:
    double wind_speed;                          // Units: m/s
    int number;                                 // Units: Beaufort scale
    std::string description;
};

#endif //NOOK_WEATHER_BEAUFORT_H
