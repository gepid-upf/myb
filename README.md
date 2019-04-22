# Move Your Body

Code provided in this repository gets the raw data from MPU-6050 and MAX30100, calculates a step counter and heart rate, and stores it in the ESP-32 SPIFFS to be send via bluetooth to an app.

## Components

To make this code work, you need the following components:

* This repository.
* [ESP32](https://espressif.com/en/products/hardware/esp32/overview) module.
* [MPU-6050](https://www.invensense.com/products/motion-tracking/6-axis/mpu-6050/) module.
* [MAX30100](https://www.maximintegrated.com/en/products/sensors/MAX30100.html) module.
* [ESP-IDF](https://github.com/espressif/esp-idf).

## Acknowledgments

This application is using code based on:

* MPU-6050 implementation for Arduino by [Jeff Rowberg](https://www.i2cdevlib.com).
* MAX30100 implementation for AVR by [Raivis Strogonovs](https://morf.lv/).