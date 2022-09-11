#include <ctime>

#include "alert.h"

WeatherAlert::WeatherAlert(std::string name, int64_t start, int64_t end) : name(name), start(start), end(end) {}

/**
 * Gets alert name
 *
 * @return alert name
 */
std::string WeatherAlert::get_name() const {
    return name;
}

/**
 * Gets information about alert start/end (starts at ... / ends at ...)
 *
 * @return information about alert start/end
 */
std::string WeatherAlert::get_time() const {
    // Get current time
    int64_t now = std::time(nullptr);
    int today = localtime((time_t *) &now)->tm_yday;

    char datetime_buf[64];

    // Compare to alert start/end time
    if (now < start) {
        tm *start_tm = localtime((time_t *) &start);
        strftime(datetime_buf, 64, today == start_tm->tm_yday ? "Starts at %H:%M" : "Starts at %a %H:%M", start_tm);
        return std::string(datetime_buf);
    } else if (now < end) {
        tm *end_tm = localtime((time_t *) &end);
        strftime(datetime_buf, 64, today == end_tm->tm_yday ? "Ends at %H:%M" : "Ends at %a %H:%M", end_tm);
        return std::string(datetime_buf);
    } else {
        tm *end_tm = localtime((time_t *) &end);
        strftime(datetime_buf, 64, today == end_tm->tm_yday ? "Ended at %H:%M" : "Ended at %a %H:%M", end_tm);
        return std::string();
    }
}
