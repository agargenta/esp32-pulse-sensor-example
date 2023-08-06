# esp-pulse-sensor-example

## Introduction
The following is an example ESP32 project aimed to showcasing the usage of the
[esp32-pulse-sensor](https://github.com/agargenta/esp32-pulse-sensor),
a ESP32-compatible C component for interfacing with pulse sensors (e.g.
[Hall-effect](https://en.wikipedia.org/wiki/Hall_effect)-based flow-meters) via GPIO (interrupts).

This example:
* Creates a queue for pulse sensor notifications
* Starts a task to consume pulse sensor notifications from this queue
* Starts the GPIO ISR service (required for the pulse sensor to work!)
* Opens a Pulse Sensor on GPIO pin 16 with min pulses per cycle at 20 and the notification queue
* Starts another task to periodically report pulse sensor totals

## Clone & Build

Clone with sub modules (dependencies):

    $ git clone --recursive https://github.com/agargenta/esp32-pulse-sensor-example.git

Build the application with:

    $ cd esp32-pulse-sensor-example
    $ idf.py set-target esp32       # or whatever you actual target is
    $ idf.py menuconfig             # adjust other options as desired
    $ idf.py build
    $ idf.py flash monitor

## Dependencies

This application makes use of the following components (included as submodules):

 * components/[esp32-pulse-sensor](https://github.com/agargenta/esp32-pulse-sensor)


## Hardware

This example assumes that you have:
* an ESP32 (DevKit) connected to your computer via USB.
* a pulse sensor of some kind (e.g. Hall-effect based flow-meters) connected to GPIO pin 16.
  * You may need to ensure correct voltage for the input signal (e.g. 5V from the sensor vs 3.3V
    expected by ESP32).

## Source Code

The source is available from [GitHub](https://www.github.com/agargenta/esp32-pulse-sensor-example).

## License

The code in this project is licensed under the MIT license - see LICENSE for details.
