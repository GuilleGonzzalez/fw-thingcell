# Thingcell firmware

This repository contains the firmware to control **Thingcell**, a device with GPS, GPRS, WiFi and Bluetooth connectivity designed for tracking or controlling external devices through a cellular network.

GPS and GPRS communication is achieved thanks to the **SIM868** module, and the ```TynyGSM``` library is used to control it.

Thingcell communication is done using ***MQTT***, making use of the ```PubSubClient``` library.

## Future work

- Make the device tracker work in home assistant.
- Specification and design of shields (battery, relay) and implementation of the functionality in the firmware.