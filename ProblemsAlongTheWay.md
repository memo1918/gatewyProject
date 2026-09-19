# Small or Big, Here Is All Problems I Have Encountered

## 1. Gateway C++/CMake issue
**Distro Package Naming:** Fedora packages Paho MQTT under paho-c-devel and paho-cpp-devel instead of paho-mqtt-c-devel

**CMake/Linker Misalignment:** CMake couldn't find the library using generic target names (paho-mqtt-cpp) because Fedora installs the binary files as libpaho-mqttpp3.so and libpaho-mqtt3as.so