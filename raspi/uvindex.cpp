#include <climits>
#include "uvindex.h"

/**
 * Creates an UVIndex object for a UV index
 *
 * @param number UV index
 */
UVIndex::UVIndex(int number) : number(number) {
    /* UV index scale */
    static const constexpr int cutoffs[5] = {2, 5, 7, 10, INT_MAX};
    const std::string descriptions[] = {
            "Low",
            "Moderate",
            "High",
            "Very high",
            "Extreme"
    };

    /* Find correct UV index description */
    for (int i = 0; i < 5; i++) {
        if (number <= cutoffs[i]) {
            number = i;
            description = descriptions[i];
            break;
        }
    }

    /* Negative values are invalid */
    if (number < 0) {
        description = "Invalid UV index";
    }
}

/**
 * Gets UV index
 *
 * @return UV index
 */
int UVIndex::get_number() const {
    return number;
}

/**
 * Gets description of UV index
 *
 * @return UV index description
 */
std::string UVIndex::get_description() const {
    return description;
}

/**
 * Gets summary to show on weather display
 *
 * @return UV index number and description
 */
std::string UVIndex::get_summary() const {
    return std::to_string(number) + " - " + description;
}