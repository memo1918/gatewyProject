#include <stdio.h>
#include <cpr/cpr.h>
#include <string>

std::string edgeDevice1 = "http://192.168.11.69:8080/";

std::string getData(std::string endpoint)
{
  cpr::Response r = cpr::Get(cpr::Url{(edgeDevice1 + endpoint)}, cpr::Timeout{50000});
  // Check network error
  if (r.error.code != cpr::ErrorCode::OK) {
    printf("Error: %s\n", r.error.message.c_str());
    return "";
  }
  // Check HTTP status code
  if (r.status_code >= 400) {
    printf("HTTP Error: %ld\n", r.status_code);
    return "";
  }

  return r.text; 
}

int main(void)
{
  std::string data = getData("sensor1");
  printf("Data: %s\n", data.c_str());
  
  return 0;
}
