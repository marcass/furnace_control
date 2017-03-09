# requires apt-get install arduino-mk
# pid library in /usr/share/arduino/libraries

ARDUINO_LIBS	= pid
BOARD_TAG	= atmega328
MONITOR_PORT	= /dev/ttyUSB0
include	 /usr/share/arduino/Arduino.mk
