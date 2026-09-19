cmake_minimum_required(VERSION 3.16)
project(Gateway CXX)

set(CMAKE_CXX_STANDARD 17)

# Find the installed packages
find_package(CURL REQUIRED)
find_package(PahoMqttCpp REQUIRED)

# 2. Define the executable
add_executable(app main.cpp)

# 3. Link the libraries
target_link_libraries(app PRIVATE 
    CURL::libcurl
    paho-mqttpp3
    paho-mqtt3as
)