#include "logger.hpp"
#include <stdio.h>
#include <cpr/cpr.h>
#include <string>

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "mqtt/async_client.h"


std::string edgeDevice1 = "http://192.168.11.69:8080/";

// MQTT broker settings
const std::string SERVER_ADDRESS = "tcp://broker.hivemq.com:1883";
const std::string CLIENT_ID = "gateway_cpp_client_123";
const std::string TOPIC_DATA_UP = "assignment/orange_pi/sensor_data";
const std::string TOPIC_CMD_DOWN = "assignment/orange_pi/commands";

// atomic<bool> creates Thread-safe flag
std::atomic<bool> is_cloud_connected{false};
std::atomic<bool> force_measure_now{false};

std::atomic<bool> worker_busy{false};
std::atomic<bool> new_data_ready{false};
std::string latest_sensor_data;
// Mutex to protect access to shared data between the main thread and the HTTP worker thread
std::mutex data_mutex;

std::vector<std::string> offline_buffer;
auto last_request_time = std::chrono::steady_clock::now();

// Function to get data from the edge device
std::string getData(std::string endpoint)
{
  
  cpr::Response r = cpr::Get(cpr::Url{(edgeDevice1 + endpoint)}, cpr::Timeout{2000});
  // Check network error
  if (r.error.code != cpr::ErrorCode::OK) {
    log_event("ERROR", "Error fetching data from edge device: " + r.error.message);
    return "";
  }
  // Check HTTP status code
  if (r.status_code >= 400) {
    log_event("ERROR", "HTTP Error fetching data from edge device: " + std::to_string(r.status_code));
    return "";
  }

  auto now = std::chrono::system_clock::now();
  auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

  // Return the response text along with the epoch timestamp
  return r.text + "," + std::to_string(epoch); 
}

// Function that runs in a separate thread to fetch data from the edge device
void fetch_data_from_edge() {
  worker_busy = true;
  
  std::string response = getData("sensor1");
  
  // Grab the mutex, safely write the data, and flip the flag
  {
      std::lock_guard<std::mutex> lock(data_mutex);
      latest_sensor_data = response;
  }
  
  new_data_ready = true;
  worker_busy = false;
}


// MQTT Classes are from the Paho MQTT C++ library Examples: async_subscribe.cpp and async_publish.cpp
// MQTT callback classes
class connection_listener : public virtual mqtt::iaction_listener {
  mqtt::async_client& cli_;

public:
  connection_listener(mqtt::async_client& cli) : cli_(cli) {}

  void on_failure(const mqtt::token& tok) override {
    log_event("ERROR", "Connection failed!");
    is_cloud_connected = false;
  }

  void on_success(const mqtt::token& tok) override {
    log_event("INFO", "Connected successfully!");
    is_cloud_connected = true;
    
    // As soon as we connect, subscribe to the commands topic
    log_event("INFO", "Subscribing to " + std::string(TOPIC_CMD_DOWN) + "...");
    cli_.subscribe(TOPIC_CMD_DOWN, 1);
  }
};

// Callback class for MQTT events    mqtt::async_client& cli_;
class action_callback : public virtual mqtt::callback {
  void connection_lost(const std::string& cause) override {
    log_event("WARN", "Connection lost: " + cause);
    is_cloud_connected = false;
  }

  void connected(const std::string& cause) override {
    log_event("INFO", "Auto-reconnected to Cloud!");
    is_cloud_connected = true;
  }

  void message_arrived(mqtt::const_message_ptr msg) override {
    log_event("INFO", "Command Arrived: " + msg->to_string());
    force_measure_now = true;
  }

  void delivery_complete(mqtt::delivery_token_ptr token) override {
    // Triggers when a message successfully reaches the Cloud
    // log_event("INFO", "Delivery complete for token: " + std::to_string(token ? token->get_message_id() : -1));
  }
};


int main() {
  // Initialize MQTT client
  mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);
  action_callback cb;
  client.set_callback(cb);
  connection_listener conn_listener(client);
  mqtt::connect_options connOpts;
  connOpts.set_clean_session(false);
  connOpts.set_automatic_reconnect(true);

  std::cout << "[Gateway] Starting connection process..." << std::endl;

  // We pass the listener so it connects in the background and doesn't block!
  client.connect(connOpts, nullptr, conn_listener);

  //non-blocking Event Loop
  while (true) {
    // Sleep for a tiny fraction of a second (100ms) to prevent 100% CPU usage
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_request_time).count();

    if ((elapsed >= 5 || force_measure_now) && !worker_busy) {
      log_event("INFO", "Triggering Edge Request...");

      force_measure_now = false; // Reset the cloud command flag
      last_request_time = now;   // Reset the 5-second timer
      
      // Spawn the HTTP worker in a detached background thread
      std::thread(fetch_data_from_edge).detach();
    }

    //offline simulation: if seconds are between 30 and 45, simulate offline
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    bool simulate_offline = (tm->tm_sec >= 30 && tm->tm_sec <= 45);

    // Check if new data is ready to be published or buffered
    if (new_data_ready) {
      std::string data_to_route;
      
      // Safely grab the data
      {
        std::lock_guard<std::mutex> lock(data_mutex);
        data_to_route = latest_sensor_data;
        new_data_ready = false; 
      }
      

      // check if cloud reachable, if yes, publish to cloud, else buffer it
      if (is_cloud_connected && !simulate_offline) {
        log_event("INFO", "Publishing to Cloud: " + data_to_route);
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC_DATA_UP, data_to_route);
        pubmsg->set_qos(1);
        client.publish(pubmsg);
      } else {
        log_event("WARN", "Offline (Simulated). Saving to buffer: " + data_to_route);
        offline_buffer.push_back(data_to_route);
      }
    }

    // buffer upload after reconnection
    if (is_cloud_connected && !simulate_offline && !offline_buffer.empty()) {
      log_event("INFO", "Reconnected! Uploading " + std::to_string(offline_buffer.size()) + " buffered messages...");
      
      for (const auto& payload : offline_buffer) {
          mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC_DATA_UP, payload);
          pubmsg->set_qos(1); // QoS 1 ensures delivery
          client.publish(pubmsg);
      }
      
      offline_buffer.clear(); // Empty the buffer now that it's sent
    }
  }

  return 0;
}
