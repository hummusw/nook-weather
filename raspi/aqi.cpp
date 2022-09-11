#include <climits>
#include "aqi.h"

/**
 * Creates an AQI object for an air quality index
 *
 * @param number air quality index
 * @param description description of air quality
 */
AQI::AQI(int number, std::string description, std::string pollutant) : number(number), description(description), pollutant(pollutant) {}

/**
 * Gets air quality index
 *
 * @return air quality index
 */
int AQI::get_number() const {
    return number;
}

/**
 * Gets description of air quality index
 *
 * @return air quality index description
 */
std::string AQI::get_description() const {
    return description;
}

/**
 * Gets summary to show on weather display
 *
 * @return air quality index number and description
 */
std::string AQI::get_summary() const {
    if (pollutant == "") {
        return std::to_string(number) + " - " + description;
    } else {
        return std::to_string(number) + " - " + description + " (" + pollutant + ")";
    }
}