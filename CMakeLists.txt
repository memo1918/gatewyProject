cmake_minimum_required(VERSION 3.16)
project(GatewayWorkspace CXX)

set(CMAKE_CXX_STANDARD 17)

# Add your sub-projects
add_subdirectory(EdgeDevice)
add_subdirectory(OtherDevice)