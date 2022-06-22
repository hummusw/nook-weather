#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstring>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <tclap/CmdLine.h>

#include "main.h"

using json = nlohmann::json;

// todo add alerts
// todo add hourly
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

/**
 * Writes data from curl's response to a string
 *
 * @param [in] contents data from curl
 * @param [in] size size of each element (always 1)
 * @param [in] nmemb number of elements/characters
 * @param [in,out] s string to save data to
 * @return number of elements handled correctly (returns 0 if error)
 */
size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *s) {
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
 * Queries openweathermap to get weather location using the OneCall API
 *
 * @param [in] lat Latitude of location
 * @param [in] lon Longitude of location
 * @param [in] appid key to use for the API call
 * @return Weather data formatted as json
 * @throws runtime_error if unable to initialize curl or unable to get weather data
 */
json get_weather_data(const double lat, const double lon, const std::string & appid) {
    /* Initialize variables */
    std::string url = "http://api.openweathermap.org/data/3.0/onecall"
                      "?lat=" + std::to_string(lat)
                      + "&lon=" + std::to_string(lon)
                      + "&exclude=minutely"
                      + "&units=metric"
                      + "&appid=" + appid;
    std::string response;

    /* Initialize curl */
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Unable to initialize curl");
    }

    /* Setup curl options */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    /* Perform curl operation and clean up */
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Unable to get weather data");
    }
    curl_easy_cleanup(curl);

    /* Parse string response as JSON and return*/
    return json::parse(response);
}


/**
 * Extracts weather data from json response
 *
 * @param [in] data json response to extract data from
 * @param [out] current CurrentWeather object to store data in
 * @param [out] precipitation Precipitation object to store data in
 * @param [out] daily Vector of five DailyWeather objects to store data in
 */
void extract_weather_data(const json & data, CurrentWeather & current, Precipitation & precipitation, std::vector<DailyWeather> & daily) {
    /* Get current conditions */
    current.timestamp = data["current"]["dt"];
    current.temp = data["current"]["temp"];
    current.feels_like = data["current"]["feels_like"];
    current.weather = data["current"]["weather"][0]["description"];
    current.icon = data["current"]["weather"][0]["icon"];
    current.wind = Beaufort((double) data["current"]["wind_speed"]);
    current.humidity = (double) data["current"]["humidity"] / 100;

    /* Get chance of precipitation */
    precipitation.hour = data["hourly"][0]["pop"];
    precipitation.today = data["daily"][0]["pop"];


    /* Get daily conditions */
    for (int i = 0; i < daily.size(); i++) {
        daily[i].timestamp = data["daily"][i]["dt"];
        daily[i].hi = data["daily"][i]["temp"]["day"];
        daily[i].lo = data["daily"][i]["temp"]["night"];
        daily[i].weather = data["daily"][i]["weather"][0]["description"];
        daily[i].icon = data["daily"][i]["weather"][0]["icon"];
    }
}

/**
 * Modifies template svg and adds in weather data
 *
 * @param [in] current data about current weather
 * @param [in] precipitation data about precipitation
 * @param [in] daily five day forecast
 * @param [in] img_dir directory of images
 * @param [in] output_svg filename of modified svg
 */
void modify_svg(const CurrentWeather & current, const Precipitation & precipitation, const std::vector<DailyWeather> & daily,
                const std::string & img_dir, const std::string & output_svg) {
    /* Read template svg */
    xmlKeepBlanksDefault(0);  // this gets rid of whitespace text elements
    xmlDocPtr doc;
    doc = xmlReadFile(&(img_dir + "templatev1.svg")[0], nullptr, 0);
    if (doc == nullptr) {
        throw std::runtime_error("Failed to read template file");
    }

    /* Set up variables */
    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    xmlNodePtr curr_node;
    xmlChar *attr_href = (xmlChar *) "href";
    xmlAttr *curr_attr;

    /* Enter into the current date group and modify values */
    curr_node = root_element->children->next->next->children;
    char datetime_buf[64];
    strftime(datetime_buf, 64, "%A, %B %e, %Y", localtime((time_t *) &(current.timestamp)));
    xmlNodeSetContent(curr_node, (xmlChar *) datetime_buf);
    curr_node = curr_node->parent;

    /* Enter into the current weather group and modify values */
    curr_node = curr_node->next->children;
    curr_node = curr_node->next;  // curr_node is now "updated at"
    strftime(datetime_buf, 64, "%R", localtime((time_t *) &(current.timestamp)));
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::string(datetime_buf))[0]);
    curr_node = curr_node->next;  // curr_node is now wind
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::to_string(current.wind.get_number()) + " - " + current.wind.get_description())[0]);
    curr_node = curr_node->next;  // curr_node is now humidity
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::to_string((int) std::round(current.humidity * 100)) + "%")[0]);
    curr_node = curr_node->next;  // curr_node is now feels like
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::to_string((int) std::round(current.feels_like)) + "°")[0]);
    curr_node = curr_node->next;  // curr_node is now temp
    xmlNodeSetContent(curr_node, (xmlChar *) &(std::to_string((int) std::round(current.temp)) + "°")[0]);
    curr_node = curr_node->next;  // curr_node is now weather description
    xmlNodeSetContent(curr_node, (xmlChar *) &(current.weather)[0]);
    curr_node = curr_node->next;  // curr_node is now icon
    for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, attr_href) != 0; curr_attr = curr_attr->next);
    if (!curr_attr) {
        throw std::runtime_error("can't find href attribute");
    }
    xmlNodeSetContent(curr_attr->children, (xmlChar *) &(current.icon + ".svg")[0]);  // href
    xmlNodeSetContent(curr_attr->next->children, (xmlChar *) &(current.icon + ".svg")[0]);  // xlink-href
    curr_node = curr_node->parent;

    /* Enter into the precipitation group and modify values */
    curr_node = curr_node->next->children;
    curr_node = curr_node->next;  // curr_node is now 1 hour pop
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::to_string((int) std::round(precipitation.hour * 100)) + "%")[0]);
    curr_node = curr_node->next;  // curr_node is now today pop
    xmlNodeSetContent(curr_node->children, (xmlChar *) &((char *) curr_node->children->content + std::to_string((int) std::round(precipitation.today * 100)) + "%")[0]);
    curr_node = curr_node->next;  // curr_node is now icon
    xmlChar *attr_opacity = (xmlChar *) "opacity";
    for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, attr_opacity) != 0; curr_attr = curr_attr->next);
    if (!curr_attr) {
        throw std::runtime_error("can't find opacity attribute");
    }
    xmlNodeSetContent(curr_attr->children, (xmlChar *) &std::to_string(std::max(precipitation.hour, precipitation.today))[0]);
    xmlNodeSetContent(curr_attr->next->children, (xmlChar *) &"umbrella.svg");
    xmlNodeSetContent(curr_attr->next->next->children, (xmlChar *) &"umbrella.svg");
    curr_node = curr_node->parent;

    /* Enter into the hourly forecast group and modify values */
    curr_node = curr_node->next->children;
    NULL;  // todo figure this stuff out
    curr_node = curr_node->parent;

    /* Enter into the daily forecast group and modify values */
    curr_node = curr_node->next->children;  // curr_node is now day 0 dow
    for (int i = 0; i < 5; i++) {
        strftime(datetime_buf, 64, "%a", localtime((time_t *) &(daily[i].timestamp)));
        xmlNodeSetContent(curr_node, (xmlChar *) datetime_buf);
        curr_node = curr_node->next;  // curr_node is now day i hi/lo
        xmlNodeSetContent(curr_node, (xmlChar *) &(std::to_string((int) std::round(daily[i].hi)) + "°/" + std::to_string((int) std::round(daily[i].lo)) + "°")[0]);
        curr_node = curr_node->next;  // curr_node is now day i icon
        for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, attr_href) != 0; curr_attr = curr_attr->next);
        if (!curr_attr) {
            throw std::runtime_error("can't find href attribute");
        }
        xmlNodeSetContent(curr_attr->children, (xmlChar *) &(daily[i].icon + ".svg")[0]);
        xmlNodeSetContent(curr_attr->next->children, (xmlChar *) &(daily[i].icon + ".svg")[0]);
        curr_node = curr_node->next;
    }

    /* Save changes to a new svg file */
    xmlSaveFileEnc(&(img_dir + output_svg)[0], doc, "UTF-8");
    xmlFreeDoc(doc);
}

int main(int argc, char *argv[]) {
    try {
        /* Get project directory path */
        std::string path = std::filesystem::canonical("/proc/self/exe").remove_filename().string() + "../";

        /* Set up command line parser */
        TCLAP::CmdLine cmd("Gathers weather information from openweathermap and generates an svg image for use on a Nook Simple Touch",
                           '=',
                           "0.1");
        TCLAP::ValueArg<double> arg_lat("", "lat", "location latitude", true, 0, "double/float", cmd);
        TCLAP::ValueArg<double> arg_lon("", "lon", "location longitude", true, 0, "double/float", cmd);
        TCLAP::ValueArg<std::string> arg_key("", "key", "api key", false, "", "string", cmd);
        // TCLAP::ValueArg<std::string> arg_output("", "output-file", "name of generated svg file", false, "generated.svg", "string", cmd);

        /* Get variables from user */
        cmd.parse(argc, argv);
        double lat = arg_lat.getValue();
        double lon = arg_lon.getValue();
        std::string apikey = arg_key.getValue().empty() ? get_apikey(path + "apikey.txt") : arg_key.getValue();
        std::string img_dir = path + "img/";
        std::string output_file = "generated.svg";

        /* Get information from OpenWeatherMap */
        json response_j = get_weather_data(lat, lon, apikey);

        /* Extract information from response */
        CurrentWeather currentWeather = {0, NAN, NAN, "", "", Beaufort(NAN), NAN};
        Precipitation precipitation = {NAN, NAN};
        std::vector<DailyWeather> fiveDays(5);
        extract_weather_data(response_j, currentWeather, precipitation, fiveDays);

        /* Use extracted information to create a svg */
        modify_svg(currentWeather, precipitation, fiveDays, img_dir, output_file);

        /* Post-processing handled by bash script */
        return 0;
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}
