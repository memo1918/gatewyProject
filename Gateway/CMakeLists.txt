cmake_minimum_required(VERSION 3.16)
project(Gateway CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# Find the installed packages
find_package(PahoMqttCpp REQUIRED)

# FetchContent for cpr
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG        1.14.2 # Replace with the latest version if needed
)

# Turn off tests
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Turn off libpsl (THIS stops the Meson/Ninja error)
set(CPR_USE_SYSTEM_LIB_PSL OFF CACHE BOOL "" FORCE)
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cpr)

# Define the executable
add_executable(gateway main.cpp
    logger.hpp
    logger.cpp)

# Link the libraries
target_link_libraries(gateway PRIVATE 
    paho-mqttpp3
    paho-mqtt3as
    cpr::cpr
)