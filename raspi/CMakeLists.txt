cmake_minimum_required(VERSION 3.22)
project(nook-weather)

set(CMAKE_CXX_STANDARD 17)

add_executable(nook-weather main.cpp beaufort.cpp)

include_directories("/usr/include/librsvg-2.0")
include_directories("/usr/include/libxml2")

target_link_libraries(nook-weather curl)
target_link_libraries(nook-weather xml2)
