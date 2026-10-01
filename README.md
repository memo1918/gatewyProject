# Gateway Cloud Project
**Edge Device (OrangePi):** Runs a simple, blocking HTTP Server. Its only job is to wait for a request, read the sensor, and reply.

**Gateway (Laptop):** Acts as the "Brain." It runs an HTTP Client (to talk down to the Edge) and an MQTT Client (to talk up to the Cloud).

**Cloud (AWS IoT Core / EC2):** Acts as the MQTT Broker. It receives scheduled data and publishes "Measure Now" commands down to the Gateway.



### Sensor Data Flow
Edge Device -> Gateway -> Cloud

Data:
Orangepi tempeture senesor data located in /sys/class/hwmon
SOC: hwmon0/temp1_input
GPU: hwmon1/temp1_input

### Remote Control Flow
Cloud -> Gateway -> Edge Device



### Communication Protocols
Edge Device <-HTTP-> Gateway
Gateway <-MQTT-> Cloud

Cloud is also MQTT Broker

## Core Logic Notes

1. Gateway maintains a state flag. This flag shows if we are currently waiting responce from Edge Device. ie "is_waiting_for_edge"

2. Handling "Measure Now": Gateway checks if we are already waiting for responce from the Edge Device. If yes we simply wait and return that. If not we send a new request to the Edge Device and wait and return that. (Edge receives http requests one at a time)

3. Command Duplication: Gateway sets a flag when reciving request from cloud, as long as this is set true (meaning we have not replied back) new messages are ignored.

4. gateway's get sensor data and wait listen for mqtt from cloud shold run async.

## Requirements
1. Core Data Flow (Upwards)

    The Gateway must periodically request/acquire sensor data from the Edge device.

    The Gateway must transmit that collected data to the Cloud.

2. Network Resilience (Offline Buffering)

    If the network connection drops, the system must not lose any data.

    Once the connection is restored, the Gateway must automatically resend the buffered data to the Cloud.

3. Remote Control (Downwards)

    The Cloud must be able to send an "Immediate Measurement" (Measure Now) command down to the Gateway.

    The Gateway must receive this, trigger the measurement on the Edge device, and handle the result.

4. Specific Edge Cases to Handle
Throughout all of the above, your logic must explicitly account for:

    Duplicate commands: What happens if the Cloud sends the "Measure Now" instruction multiple times in a row?

    Timeouts: What happens if the Gateway asks the Edge for data, but the Edge is unresponsive?

    Reconnections: How does the system behave when a broken connection (Edge-to-Gateway or Gateway-to-Cloud) comes back online?

    Logging: Keeping a record of these events, errors, and data flows.


    ## Resources
    https://yhirose.github.io/cpp-httplib/en/
    https://curl.se/libcurl/c/libcurl-tutorial.html
    https://github.com/eclipse-paho/paho.mqtt.cpp/tree/master/examples
    https://eclipse.dev/paho/files/cppdoc/index.html