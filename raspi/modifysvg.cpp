#include <iomanip>
#include <iostream>
#include <cmath>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "modifysvg.h"

/**
 * Converts a decimal number to a percentage string rounded to the nearest percent
 *
 * @param [in] value number to convert (where 1.0 = 100%)
 * @return string reprenting the percentage
 */
std::string double_to_percent(const double value) {
    return std::to_string((int) std::round(value * 100)) + "%";
}

/**
 * Rounds a number to the nearest integer and adds a degree symbol
 *
 * @param [in] temperature number to round
 * @return string representing the temperature
 */
std::string double_to_degree(const double temperature) {
    return std::to_string((int) std::round(temperature)) + "°";
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
    strstm << std::put_time(localtime((time_t *) &(current.timestamp)), "%R");
    xmlNodeAddContent(curr_node->children, (xmlChar *) strstm.str().c_str());
    curr_node = curr_node->next;

    // Air quality
    xmlNodeAddContent(curr_node->children, (xmlChar *) current.aqi.get_summary().c_str());
    curr_node = curr_node->next;

    // Wind conditions
    xmlNodeAddContent(curr_node->children, (xmlChar *) current.wind.get_summary().c_str());
    curr_node = curr_node->next;

    // UV Index
    xmlNodeAddContent(curr_node->children, (xmlChar *) current.uvi.get_summary().c_str());
    curr_node = curr_node->next;

    // Humidity
    xmlNodeAddContent(curr_node->children, (xmlChar *) double_to_percent(current.humidity).c_str());
    curr_node = curr_node->next;

    // Feels like
    xmlNodeAddContent(curr_node->children, (xmlChar *) double_to_degree(current.feels_like).c_str());
    curr_node = curr_node->next;

    // Temperature
    xmlNodeSetContent(curr_node, (xmlChar *) double_to_degree(current.temp).c_str());
    curr_node = curr_node->next;

    // Weather description
    xmlNodeSetContent(curr_node, (xmlChar *) current.weather.c_str());
    curr_node = curr_node->next;

    // Icon
    xmlSetProp(curr_node, (xmlChar *) "href", (xmlChar *) (current.icon + ".svg").c_str());
    xmlSetProp(curr_node, (xmlChar *) "xlink:href", (xmlChar *) (current.icon + ".svg").c_str());
}

/**
 * Modifies template svg to add in precipitation data
 *
 * @param [in,out] group_ptr pointer to the group-precipitation <g> node
 * @param [in] precipitation precipitation data
 */
void modify_svg_precipitation(xmlNodePtr & group_ptr, const Precipitation & precipitation) {
    xmlNodePtr curr_node = group_ptr->children;
    std::stringstream strstm;

    // Skip header
    curr_node = curr_node->next;

    // 1 hour pop
    xmlNodeAddContent(curr_node->children, (xmlChar *) double_to_percent(precipitation.hour).c_str());
    curr_node = curr_node->next;

    // Today pop
    xmlNodeAddContent(curr_node->children, (xmlChar *) double_to_percent(precipitation.today).c_str());
    curr_node = curr_node->next;

    // Icon
    xmlSetProp(curr_node, (xmlChar *) "opacity", (xmlChar *) std::to_string(std::max(precipitation.hour, precipitation.today)).c_str());
    xmlSetProp(curr_node, (xmlChar *) "href", (xmlChar *) "umbrella.svg");
    xmlSetProp(curr_node, (xmlChar *) "xlink:href", (xmlChar *) "umbrella.svg");
}

/**
 * Modifies template svg to add in hourly data
 *
 * @param [in, out] group_ptr pointer to the group-hourly <g> node
 * @param [in] hourly hourly forecast data
 */
void modify_svg_hourly(xmlNodePtr & group_ptr, const std::vector<HourlyWeather> & hourly) {
    xmlNodePtr curr_node = group_ptr->children;
    std::stringstream strstm;

    // Gather metadata about hourly forecast
    const int round_to = 5;  // Round to multiples of 5
    double temp_max = hourly[0].temp;
    double temp_min = hourly[0].temp;
    for (const HourlyWeather& hour : hourly) {
        temp_max = std::max(temp_max, hour.temp);
        temp_min = std::min(temp_min, hour.temp);
    }
    int temp_max_rounded = ceil(temp_max);
    int temp_min_rounded = floor(temp_min);
    if (temp_max_rounded % round_to != 0) {
        temp_max_rounded += (round_to - temp_max_rounded % round_to);
    }
    if (temp_min_rounded % round_to != 0) {
        temp_min_rounded -= temp_min_rounded % round_to;
    }

    // Set up constants for drawing components
    const unsigned int hours = 12;
    const unsigned int graph_bounds[2][2] = {{550, 770}, {360, 460}};  // index 0: x (0) or y (1)  index 1: start (0) or end (1)
    enum dimension {X, Y};
    enum limit {START, END};
    const unsigned int padding_lines = 10;  // amount lines extend past graph
    const unsigned int colwidth = (graph_bounds[X][END] - graph_bounds[X][START]) / (hours - 1);  // width of each column
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
    xmlSetProp(curr_node, (xmlChar *) "points", (xmlChar *) strstm.str().c_str());
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
    int divisions = (temp_max_rounded - temp_min_rounded) / round_to;
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
    xmlNodeSetContent(curr_node, (xmlChar *) double_to_degree(temp_max_rounded).c_str());
    curr_node = curr_node->next;

    xmlNodeSetContent(curr_node, (xmlChar *) double_to_degree(temp_min_rounded).c_str());
    curr_node = curr_node->next;

    for (int i = 0; i < divisions - 1; i++) {
        xmlNodePtr new_temp = xmlNewNode(nullptr, (xmlChar *) "text");
        xmlNewProp(new_temp, (xmlChar *) "class", (xmlChar *) "hourlytemp");
        xmlNewProp(new_temp, (xmlChar *) "x", (xmlChar *) &(std::to_string(graph_bounds[X][START] - padding_lines - padding_text))[0]);
        xmlNewProp(new_temp, (xmlChar *) "y", (xmlChar *) &(std::to_string(graph_bounds[Y][START] + (i + 1) * graph_height / divisions))[0]);
        xmlNodeSetContent(new_temp, (xmlChar *) double_to_degree(temp_max_rounded - (i + 1) * round_to).c_str());
        xmlAddPrevSibling(curr_node, new_temp);
    }

    // Show temperature graph
    for (int i = 0; i < hours - 1; i++) {
        double start_y = graph_bounds[Y][END] - graph_height * (hourly[i].temp - temp_min_rounded) / (temp_max_rounded - temp_min_rounded);
        double end_y = graph_bounds[Y][END] - graph_height * (hourly[i + 1].temp - temp_min_rounded) / (temp_max_rounded - temp_min_rounded);
        xmlSetProp(curr_node, (xmlChar *) "y1", (xmlChar *) std::to_string(start_y).c_str());
        xmlSetProp(curr_node, (xmlChar *) "y2", (xmlChar *) std::to_string(end_y).c_str());
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
    std::stringstream strstm = std::stringstream();

    // Fill out as many boxes as possible, up to 5 (max)
    for (int i = 0; i < std::min(5, (int) daily.size()); i++) {
        // day of week
        strstm = std::stringstream();
        strstm << std::put_time(localtime((time_t *) &(daily[i].timestamp)), "%a");
        xmlNodeSetContent(curr_node, (xmlChar *) strstm.str().c_str());
        curr_node = curr_node->next;

        // High / low
        xmlNodeSetContent(curr_node, (xmlChar *) (double_to_degree(daily[i].hi) + "/" + double_to_degree(daily[i].lo)).c_str());
        curr_node = curr_node->next;

        // Icon
        xmlSetProp(curr_node, (xmlChar *) "href", (xmlChar *) (daily[i].icon + ".svg").c_str());
        xmlSetProp(curr_node, (xmlChar *) "xlink:href", (xmlChar *) (daily[i].icon + ".svg").c_str());
        curr_node = curr_node->next;
    }
}


/**
 * Modifies template svg to add in alerts, if needed
 *
 * @param [in,out] group_ptr pointer to the group-alerts <g> node
 * @param [in] alerts alerts to display
 */
void modify_svg_alerts(xmlNodePtr & group_ptr, const std::vector<WeatherAlert> & alerts) {
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
        xmlSetProp(curr_node, (xmlChar *) "style", (xmlChar *) "fill:black");
        curr_node = curr_node->next;

        curr_node = curr_node->children;

        // Show 1 alert (show name and time)
        if (alerts.size() == 1) {
            xmlNodeSetContent(curr_node, (xmlChar *) alerts[0].get_name().c_str());
            std::stringstream strstm = std::stringstream();
            strstm << "(" << alerts[0].get_time() << ")";
            xmlNodeSetContent(curr_node->next, (xmlChar *) strstm.str().c_str());
        }

        // or show 2 alerts (show both names)
        else if (alerts.size() == 2) {
            xmlNodeSetContent(curr_node, (xmlChar *) alerts[0].get_name().c_str());
            xmlNodeSetContent(curr_node->next, (xmlChar *) alerts[1].get_name().c_str());
        }

        // or show many alerts (show name and how many others there are)
        else {
            xmlNodeSetContent(curr_node, (xmlChar *) alerts[0].get_name().c_str());
            std::stringstream strstm = std::stringstream();
            strstm << "(" << alerts.size() - 1 << " more alerts)";
            xmlNodeSetContent(curr_node->next, (xmlChar *) strstm.str().c_str());
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
                const std::vector<WeatherAlert> & alerts,
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