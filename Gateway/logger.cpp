#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

// 'static' keeps this variable completely private to this file
static std::ofstream log_file("/var/log/gateway_events.log", std::ios::app);

void log_event(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    // Print to screen
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " 
              << "[" << level << "] " << message << std::endl;

    // Write to file and flush immediately so it saves even if the program crashes
    if (log_file.is_open()) {
        log_file << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " 
                 << "[" << level << "] " << message << std::endl;
    }
}