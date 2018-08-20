# furnace_control
Control a FUWI pellet boiler with arduino to improve smarts

After having 2 controal boards malfunction I have decided to roll my own with some improved modulation of fan speed and pellet feeding. Reporting or interface issues will be addressed also

Goals satisfied with this project are (not limited to):
* Reducing the need for cleaning of pellet boiler through better end of heat cycle managemtn of burner box (basically fanning the pellet wad until it burns out)
* Increasing efficiency of pellet usage by varying energy output of pellet boilder in accordance with demand (pid controlling fan, pellet feed time and pause feed time around setpoint). This has the added benefit of avoiding short-cycling and resultant ash buildup

#Components of system
1. Pellet boiler
2. Sensor network for house (managed by https://github.com/home-climate-control/dz ) - this will pull a pin low on arduino in boiler
3. Arduino controlled PCB in boiler
4. Communicatons server (communicates to arduino via usb-serial port and the rest of the world via MQTT)

This repository deals with the arduino sketch, the PCB and the communication interface on the server

# To perform headless-server update of arduino sketch
1. Stop python listener script on serial port "sudo systemctl stop arduino-attach.service"
2. Upload script to arduino: "make upload" (in root of git repo for furnace_control)
3. Start python listener script on serial port: "sudo systemctl start arduino-attach.service"

## Inputs from boiler
* Water temperature
* Auger shaft temperature
* Light sensor in flame box

## Outputs to boiler and assocaited hardware
* Auger motor to move pellets into flame box
* Fan to stoke fire
* 280W eleement to start fire
* Circ pump to pump heated water to radiators

## Safety
Hardware fail-safes are ideal as software may have problems that result in uncontrolled fire! Currently the boiler uses a device that breaks the neutral feed to the fan and element so it does not provide continuance when it is over a certain termperature (90 degrees C)
 * Error state pulls all relays to low
 * Idle state pulls fan and element to low

## States
1. Idle
2. Start_up
3. Heating
4. Cool_down
5. Error
6. Off

### Idle
 Wait for closed contact from 1-wire bus to transition into start_up. When contact closed: Blow fan to see if flame detected:
* if flame detected -> Heating
* if no flame -> Start_up

### Start up
1. Blow fan first to see if a flame will start
1. Pellet dump to get something for element to light
2. Element heating of pellet load
3. Gentle fan (phase angled to 30% or so) to start fire until light over threshold sensed
4. Pellet feed and full fan when light over threshold
5. Transition into heating when water temp over 50C

### Heating
1. PID control of trend of water temp and this affects fan speed through PWM and pellet feed rate
2. Start circ pump if over 50C
3. Pull fan relay low and circ pump high if temp over 85C
4. Transition to cool down if contact from 1-wire is opened

### Cool_down
1. Pull fan low and circ pump high until temp <60C
2. Transition to idle when below 60C

### Error
 Catch all that pulls fan adn element low


### Off
Don't start the boiler even if dz asks (button press and remote)

##Links
* Triac schematic for fan control (need up to 5A as heat diminishes capability
https://www.circuitar.com/media/product/25/files/Triac_v1.0.pdf

* Phase control for fan:
http://playground.arduino.cc/Main/ACPhaseControl
* ACPWM lib
http://playground.arduino.cc/Code/ACPWM
* Phase control explanation with varying of power output suggestions: http://electronics.stackexchange.com/questions/19273/when-should-i-fire-the-triacs-gate-when-controlling-an-inductive-load-ac-motor

##Connecton notes
* redundant fan_out connection at relay block in case on-board pahse angle not working
* P6 (240v outs) have the hardware thermostat (AB03 90.05) connected in pins 3 and 4 (polarity doesn't matter
  * when below 90C it is closed circuit
  * when above 90C it is open circuit
  * after opening it closes again at 75C
  * this (when closed circuit) supplies 240N to pins 1, 2 and 3 (pump, auger and element)
* 1-wire relay (hobby boards 8 channel) connects at pins 7 and 8 of P1 (low voltage input connector). When it is closed by control software the DZ_pin is pulled to ground
* 15k Ohm resistors need to be rated for 5w on 240VAC side (consumes 3.8W of power)
* 1.5k Resistor needs to be 50W on 240VAC side (consumes 38W of power) if constant power, but as MOC305 datasheet states:

"The power dissapation of this current limiting resistor and the triac driver is very small because the power triac carris the load current as soon as the current through the driver and current limiting resistor reaches the trigger current of teh power triac"

Switching times for the driver are 1 micro second and 4 micro seconds for the power triac

#Setup
Components of the sytsem are:
1. Boiler controller (arduino based on a PCB)
2. Communication interface on serial port
3. Display device(s)

##Boiler controller
PCB in kicad files in this repo
* Could do with 2 additional input pins for sensing hopper level
* Check that 5W resistors are up to the task of phase angle detection

## Communication interface on serial port
Uses:
* pyserial to capture data/print data to serial port (arduino) and insert into database
* paho-mqtt for subscribing/publishing to mosquitto broker
* mosquitto broker for managing infomration

###UDEV
See udev rules in repo:
* On insertion of aruduino usb assign symlink so pyserial can find device
* runs systemd service that starts the pythion script for managing stuff to and from arduino

###Data capture
Inserting data into influxdb and graphing via a vuejs frontend using plotly. See https://skibo.duckdns.org/sensors/#/graphs (repo here: https://github.com/marcass/esp32-sensornet/tree/master/sensornet)


##Device
Any mqtt client that runs on anything will display data from broker
* subscribe to topics:
  * boiler/switch (for turning on/off)
  * boiler/flame (for displaying flame value)
  * boiler/temp/auger
  * boiler/temp/water
  * boiler/temp/setpoint
  * boiler/messages (error messages)
  * boiler/state (state display)
