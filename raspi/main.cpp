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
    // Initialize variables
    std::stringstream urlstream = std::stringstream();
    urlstream << "https://api.openweathermap.org/data/3.0/onecall"
                 "?lat=" << lat << "&lon=" << lon << "&exclude=minutely&units=metric&appid=" << appid;
    std::string response;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Unable to initialize curl");
    }

    // Setup curl options
    curl_easy_setopt(curl, CURLOPT_URL, urlstream.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform curl operation and clean up
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Unable to get weather data");
    }
    curl_easy_cleanup(curl);

    // Parse string response as JSON and return
    return json::parse(response);
}


/**
 * Extracts weather data from json response
 *
 * @param [in] data json response to extract data from
 * @param [out] current CurrentWeather object to store data in
 * @param [out] precipitation Precipitation object to store data in
 * @param [out] hourly Vector of HourlyWeather objects to store data in
 * @param [out] daily Vector of DailyWeather objects to store data in
 * @param [out] alerts Vector of strings to store alerts in
 */
void extract_weather_data(const json & data, CurrentWeather & current, Precipitation & precipitation,
                          std::vector<HourlyWeather> & hourly, std::vector<DailyWeather> & daily,
                          std::vector<std::string> & alerts) {
    // Get current conditions
    current.timestamp = data["current"]["dt"];
    current.temp = data["current"]["temp"];
    current.feels_like = data["current"]["feels_like"];
    current.weather = data["current"]["weather"][0]["description"];
    current.icon = data["current"]["weather"][0]["icon"];
    current.wind = Beaufort((double) data["current"]["wind_speed"]);
    current.humidity = (double) data["current"]["humidity"] / 100;

    // Get chance of precipitation
    precipitation.hour = data["hourly"][0]["pop"];
    precipitation.today = data["daily"][0]["pop"];

    // Get hourly conditions
    for (int i = 0; i < std::min(hourly.size(), data["hourly"].size()); i++) {
        hourly[i].timestamp = data["hourly"][i]["dt"];
        hourly[i].temp = data["hourly"][i]["temp"];
        hourly[i].pop = data["hourly"][i]["pop"];
        hourly[i].icon = data["hourly"][i]["weather"][0]["icon"];
    }

    // Get daily conditions
    for (int i = 0; i < std::min((int) daily.size(), (int) data["daily"].size()); i++) {
        daily[i].timestamp = data["daily"][i]["dt"];
        daily[i].hi = data["daily"][i]["temp"]["day"];
        daily[i].lo = data["daily"][i]["temp"]["night"];
        daily[i].weather = data["daily"][i]["weather"][0]["description"];
        daily[i].icon = data["daily"][i]["weather"][0]["icon"];
    }

    // Get alerts, if any
    if (data.contains("alerts")) {
        for (int i = 0; i < data["alerts"].size(); i++) {
            alerts.emplace_back(data["alerts"][i]["event"]);
        }
    }
}

/**
 * Modifies template svg to add in the current date
 *
 * @param [in,out] group_ptr pointer to the group-date <g> node
 * @param [in] timestamp current timestamp as unix time
 */
void modify_svg_date(xmlNodePtr & group_ptr, const uint64_t timestamp) {
    xmlNodePtr curr_node = group_ptr->children;
    char datetime_buf[64];
    strftime(datetime_buf, 64, "%A, %B %e, %Y", localtime((time_t *) &(timestamp)));
    xmlNodeSetContent(curr_node, (xmlChar *) datetime_buf);
}

/**
 * Modifies temlate svg to add in current weather conditions
 *
 * @param [in,out] group_ptr pointer to the group-current <g> node
 * @param [in] current current weather conditions
 */
void modify_svg_current(xmlNodePtr & group_ptr, const CurrentWeather & current) {
    xmlNodePtr curr_node = group_ptr->children;
    xmlAttr *curr_attr;
    std::stringstream strstm;

    // Skip header
    curr_node = curr_node->next;

    // Updated at
    strstm = std::stringstream();
    strstm << curr_node->children->content << std::put_time(localtime((time_t *) &(current.timestamp)), "%R");
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Wind conditions
    strstm = std::stringstream();
    strstm << curr_node->children->content << current.wind.get_number() << " - " << current.wind.get_description();
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Humidity
    strstm = std::stringstream();
    strstm << curr_node->children->content << (int) std::round(current.humidity * 100) << "%";
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Feels like
    strstm = std::stringstream();
    strstm << curr_node->children->content << (int) std::round(current.feels_like) << "°";
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Temperature
    strstm = std::stringstream();
    strstm << (int) std::round(current.temp) << "°";
    xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Weather description
    xmlNodeSetContent(curr_node, (xmlChar *) current.weather.c_str());
    curr_node = curr_node->next;

    // Icon
    for (curr_attr = curr_node->properties;
         curr_attr && xmlStrcmp(curr_attr->name, (xmlChar *) "href") != 0; curr_attr = curr_attr->next);
    if (!curr_attr) {
        throw std::runtime_error("can't find href attribute");
    }
    xmlNodeSetContent(curr_attr->children, (xmlChar *) (current.icon + ".svg").c_str());  // href
    xmlNodeSetContent(curr_attr->next->children, (xmlChar *) (current.icon + ".svg").c_str());  // xlink-href
}

/**
 * Modifies template svg to add in precipitation data
 *
 * @param [in,out] group_ptr pointer to the group-precipitation <g> node
 * @param [in] precipitation precipitation data
 */
void modify_svg_precipitation(xmlNodePtr & group_ptr, const Precipitation & precipitation) {
    xmlNodePtr curr_node = group_ptr->children;
    xmlAttr *curr_attr;
    std::stringstream strstm;

    // Skip header
    curr_node = curr_node->next;

    // 1 hour pop
    strstm = std::stringstream();
    strstm << curr_node->children->content << (int) std::round(precipitation.hour * 100) << "%";
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Today pop
    strstm = std::stringstream();
    strstm << curr_node->children->content << (int) std::round(precipitation.today * 100) << "%";
    xmlNodeSetContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Icon
    for (curr_attr = curr_node->properties;
         curr_attr && xmlStrcmp(curr_attr->name, (xmlChar *) "opacity") != 0; curr_attr = curr_attr->next);
    if (!curr_attr) {
        throw std::runtime_error("can't find opacity attribute");
    }
    xmlNodeSetContent(curr_attr->children,
                      (xmlChar *) std::to_string(std::max(precipitation.hour, precipitation.today)).c_str());
    xmlNodeSetContent(curr_attr->next->children, (xmlChar *) "umbrella.svg");
    xmlNodeSetContent(curr_attr->next->next->children, (xmlChar *) "umbrella.svg");
}

/**
 * Modifies template svg to add in hourly data
 *
 * @param [in, out] group_ptr pointer to the group-hourly <g> node
 * @param [in] hourly hourly forecast data
 */
void modify_svg_hourly(xmlNodePtr & group_ptr, const std::vector<HourlyWeather> & hourly) {
    xmlNodePtr curr_node = group_ptr->children;
    xmlAttr *curr_attr;
    std::stringstream strstm;

    // Gather metadata about hourly forecast
    double temp_max = hourly[0].temp;
    double temp_min = hourly[0].temp;
    for (const HourlyWeather& hour : hourly) {
        temp_max = std::max(temp_max, hour.temp);
        temp_min = std::min(temp_min, hour.temp);
    }
    int temp_max_rounded = ceil(temp_max);
    int temp_min_rounded = floor(temp_min);
    if (temp_max_rounded % 5 != 0) {
        temp_max_rounded += (5 - temp_max_rounded % 5);
    }
    if (temp_min_rounded % 5 != 0) {
        temp_min_rounded -= temp_min_rounded % 5;
    }

    // Set up constants for drawing components
    const unsigned int hours = 12;
    const unsigned int graph_bounds[2][2] = {{550, 770}, {360, 460}};  // index 0: x (0) or y (1)  index 1: start (0) or end (1)
    enum dimension {X, Y};
    enum limit {START, END};
    const unsigned int padding_lines = 10;  // amount lines extend past graph
    const unsigned int colwidth = (graph_bounds[X][END] - graph_bounds[X][START]) / hours;  // width of each column
    const unsigned int graph_height = graph_bounds[Y][END] - graph_bounds[Y][START];
    const unsigned int padding_text = 4;  // padding of text around graph

    // Skip header
    curr_node = curr_node->next;

    // Probability of precipitation graph
    strstm = std::stringstream();
    for (int i = 0; i < hours; i++) {
        strstm << graph_bounds[X][START] + i * colwidth << "," << graph_bounds[Y][END] - graph_height * hourly[i].pop << " ";
    }
    strstm << graph_bounds[X][END] << "," << graph_bounds[Y][END] << " " << graph_bounds[X][START] << "," << graph_bounds[Y][END];
    for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, (xmlChar *) "points") != 0; curr_attr = curr_attr->next);
    if (!curr_attr) {
        throw std::runtime_error("can't find points attribute");
    }
    xmlNodeSetContent(curr_attr->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Show vertical gridlines every third hour
    curr_node = curr_node->next;
    int hour = localtime((time_t *) &(hourly[1].timestamp))->tm_hour;
    for (int i = 1; i < hours - 1; i++) {
        if (hour % 3 != 0) {
            xmlNodePtr delete_node = curr_node;
            curr_node = curr_node->prev;
            xmlUnlinkNode(delete_node);
            xmlFreeNode(delete_node);
        }
        hour += 1;
        curr_node = curr_node->next;
    }
    curr_node = curr_node->next;

    // Skip default horizontal gridlines
    for (int i = 0; i < 2; i++) {
        curr_node = curr_node->next;
    }

    // Generate other horizontal gridlines
    int divisions = (temp_max_rounded - temp_min_rounded) / 5;
    for (int i = 0; i < divisions - 1; i++) {
        xmlNodePtr new_line = xmlNewNode(nullptr, (xmlChar *) "line");
        xmlNewProp(new_line, (xmlChar *) "class", (xmlChar *) "hourlygrid");
        xmlNewProp(new_line, (xmlChar *) "x1", (xmlChar *) std::to_string(graph_bounds[X][START] - padding_lines).c_str());
        xmlNewProp(new_line, (xmlChar *) "y1", (xmlChar *) std::to_string(graph_bounds[Y][START] + (i + 1) * graph_height / divisions).c_str());
        xmlNewProp(new_line, (xmlChar *) "x2", (xmlChar *) std::to_string(graph_bounds[X][END] + padding_lines).c_str());
        xmlNewProp(new_line, (xmlChar *) "y2", (xmlChar *) std::to_string(graph_bounds[Y][START] + (i + 1) * graph_height / divisions).c_str());
        xmlAddPrevSibling(curr_node, new_line);
    }

    // Show hour on drawn gridlines
    for (int i = 0; i < hours; i++) {
        tm *timestamp = localtime((time_t *) &(hourly[i].timestamp));
        if (timestamp->tm_hour % 3 == 0) {
            xmlNodeSetContent(curr_node, (xmlChar *) std::to_string(timestamp->tm_hour).c_str());
        } else {
            xmlNodePtr delete_node = curr_node;
            curr_node = curr_node->prev;
            xmlUnlinkNode(delete_node);
            xmlFreeNode(delete_node);
        }
        curr_node = curr_node->next;
    }

    // Show temps on drawn gridlines
    strstm = std::stringstream();
    strstm << temp_max_rounded << "°";
    xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    strstm = std::stringstream();
    strstm << temp_min_rounded << "°";
    xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    for (int i = 0; i < divisions - 1; i++) {
        xmlNodePtr new_temp = xmlNewNode(nullptr, (xmlChar *) "text");
        xmlNewProp(new_temp, (xmlChar *) "class", (xmlChar *) "hourlytemp");
        xmlNewProp(new_temp, (xmlChar *) "x", (xmlChar *) &(std::to_string(graph_bounds[X][START] - padding_lines - padding_text))[0]);
        xmlNewProp(new_temp, (xmlChar *) "y", (xmlChar *) &(std::to_string(graph_bounds[Y][START] + (i + 1) * graph_height / divisions))[0]);
        strstm = std::stringstream();
        strstm << temp_max_rounded - (i + 1) * 5 << "°";
        xmlNodeSetContent(new_temp, (xmlChar *) strstm.str().c_str());
        xmlAddPrevSibling(curr_node, new_temp);
    }

    // Show temperature graph
    for (int i = 0; i < hours - 1; i++) {
        double start_y = graph_bounds[Y][END] - graph_height * (hourly[i].temp - temp_min_rounded) / (temp_max_rounded - temp_min_rounded);
        double end_y = graph_bounds[Y][END] - graph_height * (hourly[i + 1].temp - temp_min_rounded) / (temp_max_rounded - temp_min_rounded);
        for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, (xmlChar *) "y1") != 0; curr_attr = curr_attr->next);
        if (!curr_attr) {
            throw std::runtime_error("can't find y1 attribute");
        }
        xmlNodeSetContent(curr_attr->children, (xmlChar *) std::to_string(start_y).c_str());  // y1
        xmlNodeSetContent(curr_attr->next->next->children, (xmlChar *) std::to_string(end_y).c_str());  // y2
        curr_node = curr_node->next ? curr_node-> next : curr_node;
    }
}


/**
 * Modifies template svg to add in daily forecast
 *
 * @param [in, out] group_ptr pointer to the group-hourly <g> node
 * @param [in] daily daily forecast data
 */
void modify_svg_daily(xmlNodePtr & group_ptr, const std::vector<DailyWeather> & daily) {
    xmlNodePtr curr_node = group_ptr->children;
    xmlAttr *curr_attr;
    std::stringstream strstm = std::stringstream();

    // Fill out as many boxes as possible, up to 5 (max)
    for (int i = 0; i < std::min(5, (int) daily.size()); i++) {
        // day of week
        strstm = std::stringstream();
        strstm << std::put_time(localtime((time_t *) &(daily[i].timestamp)), "%a");
        xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
        curr_node = curr_node->next;

        // High / low
        strstm = std::stringstream();
        strstm << (int) std::round(daily[i].hi) << "°/" << (int) std::round(daily[i].lo) << "°";
        xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
        curr_node = curr_node->next;

        // Icon
        for (curr_attr = curr_node->properties; curr_attr && xmlStrcmp(curr_attr->name, (xmlChar *) "href") != 0; curr_attr = curr_attr->next);
        if (!curr_attr) {
            throw std::runtime_error("can't find href attribute");
        }
        xmlNodeSetContent(curr_attr->children, (xmlChar *) (daily[i].icon + ".svg").c_str());
        xmlNodeSetContent(curr_attr->next->children, (xmlChar *) (daily[i].icon + ".svg").c_str());
        curr_node = curr_node->next;
    }
}


/**
 * Modifies template svg to add in alerts, if needed
 *
 * @param [in,out] group_ptr pointer to the group-alerts <g> node
 * @param [in] alerts alerts to display
 */
void modify_svg_alerts(xmlNodePtr & group_ptr, const std::vector<std::string> & alerts) {
    xmlNodePtr curr_node = group_ptr->children;

    // Hide alerts if not needed
    if (alerts.empty()) {
        // Hide entire group
        xmlNewProp(group_ptr, (xmlChar *) "visibility", (xmlChar *) "hidden");

        // Delete everything in the group
        while (group_ptr->children) {
            xmlNodePtr delete_node = group_ptr->children;
            xmlUnlinkNode(delete_node);
            xmlFreeNode(delete_node);
        }
    }

    // Show alerts if needed
    else {
        // Make box black
        xmlNewProp(curr_node, (xmlChar *) "style", (xmlChar *) "fill:black");
        curr_node = curr_node->next;

        // Show 1 alert
        if (alerts.size() == 1) {
            xmlNodeSetContent(curr_node, (xmlChar *) alerts[0].c_str());
        }

        // or show 2 alerts
        else if (alerts.size() == 2) {
            // First show top alert
            xmlNodePtr new_tspan = xmlNewNode(nullptr, (xmlChar *) "tspan");
            xmlNewProp(new_tspan, (xmlChar *) "dy", (xmlChar *) "-0.6em");
            xmlNodeSetContent(new_tspan, (xmlChar *) alerts[0].c_str());
            xmlAddChild(curr_node, new_tspan);

            // Then show bottom alert
            new_tspan = xmlNewNode(nullptr, (xmlChar *) "tspan");
            xmlNewProp(new_tspan, (xmlChar *) "x", (xmlChar *) "640");
            xmlNewProp(new_tspan, (xmlChar *) "dy", (xmlChar *) "1.2em");
            xmlNodeSetContent(new_tspan, (xmlChar *) alerts[1].c_str());
            xmlAddChild(curr_node, new_tspan);
        }

        // or show many alerts
        else {
            // First show top alert
            xmlNodePtr new_tspan = xmlNewNode(nullptr, (xmlChar *) "tspan");
            xmlNewProp(new_tspan, (xmlChar *) "dy", (xmlChar *) "-0.6em");
            xmlNodeSetContent(new_tspan, (xmlChar *) alerts[0].c_str());
            xmlAddChild(curr_node, new_tspan);

            // Then show that there are n-1 other alerts
            new_tspan = xmlNewNode(nullptr, (xmlChar *) "tspan");
            xmlNewProp(new_tspan, (xmlChar *) "x", (xmlChar *) "640");
            xmlNewProp(new_tspan, (xmlChar *) "dy", (xmlChar *) "1.2em");
            std::stringstream strstm = std::stringstream();
            strstm << "(" << alerts.size() - 1 << " more)";
            xmlNodeSetContent(new_tspan, (xmlChar *) strstm.str().c_str());
            xmlAddChild(curr_node, new_tspan);
        }
    }
}

/**
 * Modifies template svg and adds in weather data
 *
 * @param [in] current data about current weather
 * @param [in] precipitation data about precipitation
 * @param [in] hourly hourly forecast
 * @param [in] daily daily forecast
 * @param [in] alerts alerts to show
 * @param [in] img_dir directory of images
 * @param [in] template_svg filename of template svg
 * @param [in] output_svg filename of modified svg
 */
void modify_svg(const CurrentWeather & current, const Precipitation & precipitation,
                const std::vector<HourlyWeather> & hourly, const std::vector<DailyWeather> & daily,
                const std::vector<std::string> & alerts,
                const std::string & img_dir, const std::string & template_svg, const std::string & output_svg) {
    // Read template svg
    xmlKeepBlanksDefault(0);  // this gets rid of whitespace text elements
    xmlDocPtr doc = xmlReadFile((img_dir + template_svg).c_str(), nullptr, 0);
    if (doc == nullptr) {
        throw std::runtime_error("Failed to read template file");
    }

    // Set up variables
    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    xmlNodePtr group_ptr = root_element->children->next->next;

    // Add current date
    modify_svg_date(group_ptr, current.timestamp);
    group_ptr = group_ptr->next;

    // Add current conditions
    modify_svg_current(group_ptr, current);
    group_ptr = group_ptr->next;

    // Add precipitation data
    modify_svg_precipitation(group_ptr, precipitation);
    group_ptr = group_ptr->next;

    // Add hourly forecast
    modify_svg_hourly(group_ptr, hourly);
    group_ptr = group_ptr->next;

    // Add daily forecast
    modify_svg_daily(group_ptr, daily);
    group_ptr = group_ptr->next;

    // Add alerts
    modify_svg_alerts(group_ptr, alerts);
    group_ptr = group_ptr->next;

    // Save changes to a new svg file
    xmlSaveFileEnc((img_dir + output_svg).c_str(), doc, "UTF-8");
    xmlFreeDoc(doc);
}

int main(int argc, char *argv[]) {
    try {
        // Get project directory path
        std::string path = std::filesystem::canonical("/proc/self/exe").remove_filename().string() + "../";

        // Set up command line parser
        TCLAP::CmdLine cmd("Gathers weather information from openweathermap and generates an svg image for use on a Nook Simple Touch",
                           '=',
                           "0.1");
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
        json response_j = get_weather_data(lat, lon, apikey);

        // Extract information from response
        CurrentWeather current_weather = {0, NAN, NAN, "", "", Beaufort(NAN), NAN};
        Precipitation precipitation = {NAN, NAN};
        std::vector<HourlyWeather> hourly(12);
        std::vector<DailyWeather> daily(5);
        std::vector<std::string> alerts;
        extract_weather_data(response_j, current_weather, precipitation, hourly, daily, alerts);

        // Use extracted information to create a svg
        modify_svg(current_weather, precipitation, hourly, daily, alerts, img_dir, template_file, output_file);

        // Post-processing handled by bash script
        return 0;
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}
