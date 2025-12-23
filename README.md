
#ESP32 Firmware for SHT31 interface - temperature and moisture 
The SHT31 sensor is a I2C and the Esp32 has only one i2c interface with a i2c-multiplexer sparkfun , https://www.sparkfun.com/sparkfun-qwiic-mux-breakout-8-channel-tca9548a.html; it is possible to sample on four SHT31 sensors, for measuring temperature and moisture.
the measurements are done 2 places at the front of 1 and 1.5 m and at the wall behind the elements -
Data is transmitted over by mqtt/tcp protocol via wifi to the cloud thingspeak.com 
This project is created in vscode using platformIO - and the esp32 dev 1  board is used - so the code i depended on Arduino firmware incl. the library for the sparkfun multiplexer  SparkFun Qwiic Mux Breakout - 8 Channel (TCA9548A)

https://learn.sparkfun.com/tutorials/qwiic-mux-hookup-guide


To start up - create an empty arduino project in platformio using the esp32 dev kit v 1 . compile - Next overwrite the main and include the SHTSensor files and the Sparkfun_i2c_mux_arduino_librarty  - overwrite the default platformio file with the contents found here
