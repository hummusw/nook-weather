#ifndef NOOK_WEATHER_UVINDEX_H
#define NOOK_WEATHER_UVINDEX_H

#include <string>

class UVIndex {
public:
    explicit UVIndex(int number);               // Construct UVIndex object from UV index
    int get_number() const;                     // Getter method for number
    std::string get_description() const;        // Getter method for description
    std::string get_summary() const;            // Gets summary to show on weather display
private:
    int number;                                 // Units: UV index
    std::string description;
};

#endif //NOOK_WEATHER_UVINDEX_H
