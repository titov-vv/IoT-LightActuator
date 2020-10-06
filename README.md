# ESP32 AWS IoT actuator

ESP32 draft project to receive a command from AWS IoT topic and perform action

1. Connects to WiFi network and sync with NTP
2. Subscribes to MQTT topic to get thing updates in JSON format
3. Turn lamp on and off with help of SSR G3MB-202P

Schematics:

![Electric Scheme](https://github.com/titov-vv/IoT-LightActuator/blob/master/electric_scheme.png?raw=true)
