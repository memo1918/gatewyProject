# Gateway Cloud Project

This project connects an Orange Pi to a gateway and a small cloud dashboard.

**Edge Device (Orange Pi):** Runs a simple blocking HTTP server. It reads the hardware temperature files and returns a value when the Gateway asks for one.

**Gateway (Laptop):** Acts as the middle layer. It requests measurements from the Edge Device over HTTP and publishes them to the Cloud over MQTT.

**Cloud:** Runs the dashboard and uses MQTT to receive measurements and send `Measure Now` commands to the Gateway.

## Sensor Data Flow

`Edge Device -> Gateway -> Cloud`

The Orange Pi reads temperature data from `/sys/class/hwmon`:

- SoC: `hwmon0/temp1_input`
- GPU: `hwmon1/temp1_input`

The Edge Device exposes these HTTP endpoints:

- `/sensor1` returns the SoC temperature.
- `/sensor2` returns the GPU temperature.

The Gateway currently requests `/sensor1`. The response is sent to the Cloud as:

```text
temperature,timestamp
```

## Remote Control Flow

`Cloud -> Gateway -> Edge Device`

The Cloud publishes a command to the Gateway's MQTT command topic. The Gateway then requests a fresh measurement from the Edge Device.

## Communication Protocols

- Edge Device <-> Gateway: HTTP
- Gateway <-> Cloud: MQTT

The current development setup uses HiveMQ's public MQTT broker. The Cloud application also provides the dashboard over HTTP.

## Gateway Logic

1. The Gateway checks the Edge Device every five seconds.
2. An HTTP request runs in a worker thread so the MQTT and timer loop can continue running.
3. `Measure Now` sets a flag. If several commands arrive while a request is already running, the flag keeps the next measurement request from being lost, but multiple commands are coalesced into one request.
4. An Edge Device request has a two-second timeout. A failed request is logged and the next scheduled interval can try again.
5. When the Cloud connection is unavailable, completed measurements are stored in an in-memory buffer. The Gateway publishes the buffer after reconnecting.
6. MQTT uses automatic reconnect, a persistent session (`clean_session = false`), and QoS 1 for sensor data. The buffer itself is not persistent, so it is lost if the Gateway process stops.

## Requirements

1. **Core data flow (upwards)**

    The Gateway must periodically request sensor data from the Edge Device and transmit the collected data to the Cloud.

2. **Network resilience (offline buffering)**

    If the network connection drops, the system should not lose completed measurements. Once the connection is restored, the Gateway should automatically resend buffered data to the Cloud.

3. **Remote control (downwards)**

    The Cloud must be able to send an `Immediate Measurement` (`Measure Now`) command to the Gateway. The Gateway must receive it, trigger a measurement on the Edge Device, and handle the result.

4. **Specific edge cases**

    - **Duplicate commands:** repeated commands are coalesced while a measurement is in progress.
    - **Timeouts:** the request fails after two seconds, is logged, and can be retried on the next interval.
    - **Reconnections:** MQTT reconnects automatically and buffered measurements are sent when the connection is available again.
    - **Logging:** connection events, errors, requests, and data flows are logged.

## Resources

- [cpp-httplib documentation](https://yhirose.github.io/cpp-httplib/en/)
- [libcurl tutorial](https://curl.se/libcurl/c/libcurl-tutorial.html)
- [Paho MQTT C++ examples](https://github.com/eclipse-paho/paho.mqtt.cpp/tree/master/examples)
- [Paho MQTT C++ API documentation](https://eclipse.dev/paho/files/cppdoc/index.html)