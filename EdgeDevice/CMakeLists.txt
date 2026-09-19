cmake_minimum_required(VERSION 3.16)
project(EdgeDevice CXX)

set(CMAKE_CXX_STANDARD 17)

# Thread support is required by cpp-httplib
find_package(Threads REQUIRED)

add_executable(server main.cpp)

target_link_libraries(server PRIVATE Threads::Threads)