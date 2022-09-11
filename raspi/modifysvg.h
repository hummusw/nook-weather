#ifndef NOOK_WEATHER_MODIFYSVG_H
#define NOOK_WEATHER_MODIFYSVG_H

#include <vector>
#include "weathertypes.h"

void modify_svg(const CurrentWeather & current, const Precipitation & precipitation,
                const std::vector<HourlyWeather> & hourly, const std::vector<DailyWeather> & daily,
                const std::vector<WeatherAlert> & alerts,
                const std::string & img_dir, const std::string & template_svg, const std::string & output_svg);

#endif //NOOK_WEATHER_MODIFYSVG_H
