cmake_minimum_required(VERSION 3.16)
project(nook_weather)

set(CMAKE_CXX_STANDARD 17)

add_executable(nook_weather main.cpp modifysvg.cpp aqi.cpp beaufort.cpp uvindex.cpp alert.cpp api-openweathermap.cpp)

include_directories("/usr/include/librsvg-2.0")
include_directories("/usr/include/libxml2")

target_link_libraries(nook_weather curl)
target_link_libraries(nook_weather xml2)
