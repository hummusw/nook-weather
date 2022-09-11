#ifndef NOOK_WEATHER_AQI_H
#define NOOK_WEATHER_AQI_H

#include <string>

class AQI {
public:
    explicit AQI(int number, std::string description, std::string pollutant="");    // Construct AQI object from supplied number and description
    int get_number() const;                                                         // Getter method for number
    std::string get_description() const;                                            // Getter method for description
    std::string get_summary() const;                                                // Gets summary to show on weather display
private:
    int number;                                                                     // Units: AQI (not standardized)
    std::string description;
    std::string pollutant;                                                          // Primary pollutant
};

#endif //NOOK_WEATHER_AQI_H
