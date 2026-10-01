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