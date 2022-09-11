#include <fstream>
#include <filesystem>

#include <tclap/CmdLine.h>

#include "api-openweathermap.h"
#include "modifysvg.h"

// todo rework precipitation icon
// todo differentiate between rain and snow
// todo inkscape pixelation workaround

/**
 * Gets api key stored from file
 *
 * @param [in] filepath path to the file containing the api key
 * @return api key as a string
 */
std::string get_apikey(const std::string & filepath) {
    std::ifstream file;
    std::string key;

    file.open(filepath);
    file >> key;
    return key;
}



int main(int argc, char *argv[]) {
    try {
        // Get project directory path
        std::string path = std::filesystem::canonical("/proc/self/exe").remove_filename().string() + "../";

        // Set up command line parser
        TCLAP::CmdLine cmd("Gathers weather information from openweathermap and generates an svg image for use on a Nook Simple Touch",
                           '=',
                           "0.2");
        TCLAP::ValueArg<double> arg_lat("", "lat", "location latitude", true, 0, "double/float", cmd);
        TCLAP::ValueArg<double> arg_lon("", "lon", "location longitude", true, 0, "double/float", cmd);
        TCLAP::ValueArg<std::string> arg_key("", "key", "api key", false, "", "string", cmd);
        // TCLAP::ValueArg<std::string> arg_output("", "output-file", "name of generated svg file", false, "generated.svg", "string", cmd);

        // Get variables from user
        cmd.parse(argc, argv);
        double lat = arg_lat.getValue();
        double lon = arg_lon.getValue();
        std::string apikey = arg_key.getValue().empty() ? get_apikey(path + "apikey.txt") : arg_key.getValue();
        std::string img_dir = path + "img/";
        std::string template_file = "template.svg";
        std::string output_file = "generated.svg";

        // Get information from OpenWeatherMap
        OpenWeatherMap weather_data(lat, lon, apikey);
        CurrentWeather current_weather = weather_data.get_current();
        Precipitation precipitation = weather_data.get_precipitation();
        std::vector<HourlyWeather> hourly = weather_data.get_hourly(12);
        std::vector<DailyWeather> daily = weather_data.get_daily(5);
        std::vector<WeatherAlert> alerts = weather_data.get_alerts();

        // Use extracted information to create a svg
        modify_svg(current_weather, precipitation, hourly, daily, alerts, img_dir, template_file, output_file);

        // Post-processing handled by bash script
        return 0;
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}
