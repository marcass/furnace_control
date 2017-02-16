# furnace_control
Control a FUWI pellet boiler with arduino to improve smarts

After having 2 controal boards malfunction I have decided to roll my own with some improved modulation of fan speed and pellet feeding. Reporting or interface issues will be addressed also

## Inputs
* Water temperature
* Auger shaft temperature
* Light sensor in flame box

## Outputs
* Auger motor to move pellets into flame box
* Fan to stoke fire
* 280W eleement to start fire
* Circ pump to pump heated water to radiators

## Safety
Hardware fail-safes are ideal as software may have problems that result in uncontrolled fire! Currently the boiler uses a device that breaks the neutral feed to the fan and element so it presumably does not provide continuance when it is over a certain termperature
 * Error state pulls all relays to low
 * Idle state pulls fan and element to low
 
 ## States
1. Idle
2. Start_up
3. Heating
4. Cool_down
5. Error
 
 ### Idle
 Wait for closed contact from 1-wire bus to transition into. When contact closed: Blow fan for 20s to see if flame detected:
* if flame detected -> Heating
* if no flame -> Start_up
 
 ### Start up
1. Blow fan for 20s first to see if a flame will start
1. Pellet dump for 1min?
2. Element heating for 2min?
3. Gentle fan (pwm do 20% or so) to start fire until light over threshold sensed
4. Pellet feed and full fan when light over threshold
5. Transition into heating when water temp over 60C
 
 ### Heating
1. PID control of trend of water temp and this affects fan speed through PWM and pellet feed rate
2. Start circ pump if over 60C
3. Pull fan relay low and circ pump high if temp over 85C
4. Transition to cool down if contact from 1-wire us opened
 
 ### Cool_down
1. Pull fan low and circ pump high until temp <60C
2. Transition to idle when below 60C
 
 ### Error
 Catch all that pulls fan adn element low. Probably not needed

##Links
Triac schematic for fan control (need up to 5A as heat diminishes capability
https://www.circuitar.com/media/product/25/files/Triac_v1.0.pdf

Phase control for fan:
http://playground.arduino.cc/Main/ACPhaseControl

##Connecton notes
* redundant fan_out connection at relay block in case on-board pahse angle not working
* P6 (240v outs) have the hardware thermostat (AB03 90.05) connected in pins 3 and 4 (polarity doesn't matter
  * when below 90C it is closed circuit
  * when above 90C it is open circuit
  * after opening it closes again at 75C
  * this (when closed circuit) supplies 240N to pins 1, 2 and 3 (pump, auger and element)
* 1-wire relay (hobby boards 8 channel) connects at pins 7 and 8 of P1 (low voltage input connector). When it is closed by control software the DZ_pin is pulled to ground
