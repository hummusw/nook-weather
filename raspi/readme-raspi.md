# Raspberry Pi setup notes
## Required libraries
* `libtclap-dev`
* `libcurl4-openssl-dev`
* `nlohmann-json3-dev`
* `libxml2-dev`

## Requried programs
* `inkscape`
* `imagemagick`

## Miscellaneous
The `nook-weather` executable expects to be one directory beneath the project directory. For example, if the project directory is `~/nook-weather/` and the images are located at `~/nook-weather/img/`, then make sure the executable is somewhere like `~/nook-weather/build/nook-weather`.

Also, this currently doesn't work on Windows due to the usage of `/proc/self/exe`. I'm assuming if you want to run this on a Raspberry Pi, you weren't planning on using Windows for the operating system anyway.