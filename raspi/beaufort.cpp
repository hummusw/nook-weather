#include <cfloat>
#include "beaufort.h"

/**
 * Creates a Beaufort object for a wind speed
 *
 * @param wind_speed wind speed in m/s
 */
Beaufort::Beaufort(const double wind_speed) : wind_speed(wind_speed) {
    /* Beaufort scale in m/s */
    static const constexpr double cutoffs[13] = {0.4, 1.5, 3.3, 5.5, 7.9, 10.7, 13.8, 17.1, 20.7, 24.4, 28.4, 32.6, DBL_MAX};
    const std::string descriptions[] = {
            "Calm",
            "Light air",
            "Light breeze",
            "Gentle breeze",
            "Moderate breeze",
            "Fresh breeze",
            "Strong breeze",
            "Near gale",
            "Gale",
            "Severe gale",
            "Storm",
            "Violent storm",
            "Hurricane"
    };

    /* NAN speeds are invalid */
    number = -1;
    description = "Invalid wind speed";

    /* Find correct Beaufort number */
    for (int i = 0; i < 13; i++) {
        if (wind_speed <= cutoffs[i]) {
            number = i;
            description = descriptions[i];
            break;
        }
    }

    /* Negative speeds are invalid */
    if (wind_speed < 0) {
        number = -1;
        description = "Invalid wind speed";
    }
}

/**
 * Gets stored wind speed in m/s
 *
 * @return wind speed in m/s
 */
double Beaufort::get_wind_speed() const {
    return wind_speed;
}

/**
 * Gets Beaufort number
 *
 * @return Beaufort number
 */
int Beaufort::get_number() const {
    return number;
}

/**
 * Gets description of wind conditions
 *
 * @return wind description
 */
std::string Beaufort::get_description() const {
    return description;
}

/**
 * Gets summary to show on weather display
 *
 * @return Beaufort number and description
 */
std::string Beaufort::get_summary() const {
    return std::to_string(number) + " - " + description;
}