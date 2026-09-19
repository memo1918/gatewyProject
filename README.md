# Gateway Cloud Project
Edge Device -> OrangePi 3b
Gateway -> PC
Cloud -> AWS E2C Instance



### Sensor Data Flow
Edge Device -> Gateway -> Cloud



### Remote Control Flow
Cloud -> Gateway -> Edge Device



### Communication Protocols
Edge Device <-HTTP-> Gateway
Gateway <-MQTT-> Cloud

Cloud is also MQTT Broker


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