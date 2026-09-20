#include "httplib.h"
#include <cstdlib>
#include <fstream>



void handle_soc_sensor1(const httplib::Request &req, httplib::Response &res) {
  std::ifstream soc_temp_file("/sys/class/hwmon/hwmon0/temp1_input");
  if (!soc_temp_file.is_open()) {
    res.set_content("Error opening file", "text/plain");
    return;
  }

  double raw_temp;
  soc_temp_file >> raw_temp;
  
  // Convert millidegrees C to degrees C
  double temp_celsius = raw_temp / 1000.0;

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << temp_celsius;

  res.set_content(stream.str(), "text/plain");
}

void handle_gpu_sensor1(const httplib::Request &req, httplib::Response &res) {
  std::ifstream gpu_temp_file("/sys/class/hwmon/hwmon1/temp1_input");
  if (!gpu_temp_file.is_open()) {
    res.set_content("Error opening file", "text/plain");
    return;
  }

  double raw_temp;
  gpu_temp_file >> raw_temp;

  // Convert millidegrees C to degrees C
  double temp_celsius = raw_temp / 1000.0;

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << temp_celsius;
  
  res.set_content(stream.str(), "text/plain");
}


int main() {
  httplib::Server svr;

  svr.Get("/sensor1", handle_soc_sensor1);
  svr.Get("/sensor2", handle_gpu_sensor1);

  svr.listen("0.0.0.0", 8080);
}
