# Small or Big, Here Is All Problems I Have Encountered

## 1. Gateway C++/CMake issue
**Distro Package Naming:** Fedora packages Paho MQTT under paho-c-devel and paho-cpp-devel instead of paho-mqtt-c-devel

**CMake/Linker Misalignment:** CMake couldn't find the library using generic target names (paho-mqtt-cpp) because Fedora installs the binary files as libpaho-mqttpp3.so and libpaho-mqtt3as.so

## 2. HTTP library is blocking


## 3. C vs C++ Mutex
in C you have to call unlock on mutex but c++ works based on scope with curly braces
### C
```
pthread_mutex_lock(&my_mutex);   // 1. Lock

shared_data = 42;                

pthread_mutex_unlock(&my_mutex); // 2. Unlock
```

### C++
```
{
    std::lock_guard<std::mutex> lock(my_mutex); // 1. Locks instantly
    
    shared_data = 42;
    
} // 2. Unlocks automatically
```

## MQTT connection status issue
Turns out MQTT only checks if the message is send to broker. so we have no idea if the cloud is alive or not or receiving our messages. 
Turns out broker can store data if we configure mqtt with `clean_session=False`, in this way eventhough gateway thinks its connected to the cloud and sends data, we are not loosing data. For scope of this practice, we are going to assume broker and cloud is always online and internet connection of the gateway is only break point.