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


// MQTT callback classes
class connection_listener : public virtual mqtt::iaction_listener {
    mqtt::async_client& cli_;

public:
    connection_listener(mqtt::async_client& cli) : cli_(cli) {}

    void on_failure(const mqtt::token& tok) override {
        std::cout << "[MQTT] Connection failed!" << std::endl;
        is_cloud_connected = false;
    }

    void on_success(const mqtt::token& tok) override {
        std::cout << "[MQTT] Connected successfully!" << std::endl;
        is_cloud_connected = true;
        
        // As soon as we connect, subscribe to the commands topic
        std::cout << "[MQTT] Subscribing to " << TOPIC_CMD_DOWN << "..." << std::endl;
        cli_.subscribe(TOPIC_CMD_DOWN, 1);
    }
};

// Callback class for MQTT events
class action_callback : public virtual mqtt::callback {
    void connection_lost(const std::string& cause) override {
        std::cout << "\n[MQTT] Connection lost: " << cause << std::endl;
        is_cloud_connected = false;
    }

    void connected(const std::string& cause) override {
        std::cout << "\n[MQTT] Auto-reconnected to Cloud!" << std::endl;
        is_cloud_connected = true;
        
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "\n[MQTT] Command Arrived: " << msg->to_string() << std::endl;
        force_measure_now = true;
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        // Triggers when a message successfully reaches the Cloud
        std::cout << "[MQTT] Delivery complete for token: " << (token ? token->get_message_id() : -1) << std::endl;
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
          std::cout << "[Gateway] Triggering Edge Request..." << std::endl;
          
          force_measure_now = false; // Reset the cloud command flag
          last_request_time = now;   // Reset the 5-second timer
          
          // Spawn the HTTP worker in a detached background thread
          std::thread(fetch_data_from_edge).detach();
      }


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
          if (is_cloud_connected) {
              std::cout << "[Gateway] Publishing to Cloud: " << data_to_route << std::endl;
              mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC_DATA_UP, data_to_route);
              pubmsg->set_qos(1);
              client.publish(pubmsg);
          } else {
              std::cout << "[Gateway] Offline. Saving to buffer..." << std::endl;
              offline_buffer.push_back(data_to_route);
          }
      }

      // buffer upload after reconnection
      if (is_cloud_connected && !offline_buffer.empty()) {
          std::cout << "[Gateway] Reconnected! Uploading " << offline_buffer.size() << " buffered messages..." << std::endl;
          
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
