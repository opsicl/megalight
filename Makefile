BOARD_TAG    = mega
BOARD_SUB    = atmega2560
MONITOR_PORT = /dev/ttyUSB0
#MONITOR_PORT = /dev/ttyACM0

ARDUINO_LIBS = SPI Ethernet pubsubclient ArduinoJson EEPROM




include $(ARDMK_DIR)/Arduino.mk
