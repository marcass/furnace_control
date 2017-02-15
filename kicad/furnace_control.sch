EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:arduino
LIBS:H11AA1
LIBS:furnace_control-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L arduino_mini U1
U 1 1 58A421C5
P 2600 2000
F 0 "U1" H 3100 1050 70  0000 C CNN
F 1 "arduino_nano" H 3350 950 70  0000 C CNN
F 2 "arduino_nano:ArduinoNano" H 2600 1950 60  0001 C CNN
F 3 "" H 2600 2000 60  0001 C CNN
	1    2600 2000
	1    0    0    -1  
$EndComp
Text GLabel 4050 1950 2    60   Input ~ 0
pump
Text GLabel 3550 1750 2    60   Input ~ 0
fan_out
Text GLabel 3800 2500 2    60   Input ~ 0
auger
Text GLabel 3400 2400 2    60   Input ~ 0
element
Text GLabel 1800 2100 0    60   Input ~ 0
water_temp
Text GLabel 1150 2200 0    60   Input ~ 0
auger_temp
Text GLabel 1150 2000 0    60   Input ~ 0
light
Text GLabel 3900 2300 2    60   Input ~ 0
dz_pin
Text GLabel 3500 2050 2    60   Input ~ 0
dz_supply
$Comp
L GND #PWR3
U 1 1 58A42364
P 2600 3850
F 0 "#PWR3" H 2600 3600 50  0001 C CNN
F 1 "GND" H 2600 3700 50  0000 C CNN
F 2 "" H 2600 3850 50  0000 C CNN
F 3 "" H 2600 3850 50  0000 C CNN
	1    2600 3850
	1    0    0    -1  
$EndComp
Text GLabel 5450 800  0    60   Input ~ 0
water_temp
$Comp
L GND #PWR7
U 1 1 58A42451
P 5750 2250
F 0 "#PWR7" H 5750 2000 50  0001 C CNN
F 1 "GND" H 5750 2100 50  0000 C CNN
F 2 "" H 5750 2250 50  0000 C CNN
F 3 "" H 5750 2250 50  0000 C CNN
	1    5750 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	5450 800  7550 800 
Wire Wire Line
	5750 1050 5750 800 
Connection ~ 5750 800 
$Comp
L R R1
U 1 1 58A424A5
P 5750 1200
F 0 "R1" V 5830 1200 50  0000 C CNN
F 1 "10k" V 5750 1200 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 5680 1200 50  0001 C CNN
F 3 "" H 5750 1200 50  0000 C CNN
	1    5750 1200
	1    0    0    -1  
$EndComp
Wire Wire Line
	5750 2250 5750 1350
$Comp
L GND #PWR8
U 1 1 58A425C1
P 6750 2450
F 0 "#PWR8" H 6750 2200 50  0001 C CNN
F 1 "GND" H 6750 2300 50  0000 C CNN
F 2 "" H 6750 2450 50  0000 C CNN
F 3 "" H 6750 2450 50  0000 C CNN
	1    6750 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	6450 1000 7550 1000
Wire Wire Line
	6750 1250 6750 1000
Connection ~ 6750 1000
$Comp
L R R2
U 1 1 58A425D2
P 6750 1400
F 0 "R2" V 6830 1400 50  0000 C CNN
F 1 "10k" V 6750 1400 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 6680 1400 50  0001 C CNN
F 3 "" H 6750 1400 50  0000 C CNN
	1    6750 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	6750 2450 6750 1550
Text GLabel 6450 1000 0    60   Input ~ 0
auger_temp
$Comp
L GND #PWR9
U 1 1 58A42687
P 7350 2650
F 0 "#PWR9" H 7350 2400 50  0001 C CNN
F 1 "GND" H 7350 2500 50  0000 C CNN
F 2 "" H 7350 2650 50  0000 C CNN
F 3 "" H 7350 2650 50  0000 C CNN
	1    7350 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	7350 1450 7350 1200
Connection ~ 7350 1200
$Comp
L R R3
U 1 1 58A42698
P 7350 1600
F 0 "R3" V 7430 1600 50  0000 C CNN
F 1 "10k" V 7350 1600 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 7280 1600 50  0001 C CNN
F 3 "" H 7350 1600 50  0000 C CNN
	1    7350 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	7350 2650 7350 1750
Text GLabel 7050 1200 0    60   Input ~ 0
light
Wire Wire Line
	3550 1750 3300 1750
Wire Wire Line
	3500 2050 3300 2050
Wire Wire Line
	3900 2300 3300 2300
Wire Wire Line
	3400 2400 3300 2400
Wire Wire Line
	3800 2500 3300 2500
Wire Wire Line
	1150 2000 1900 2000
Wire Wire Line
	1900 2100 1800 2100
Wire Wire Line
	1900 2200 1150 2200
Wire Wire Line
	2600 3850 2600 3550
Text GLabel 8550 1350 0    60   Input ~ 0
pump
Text GLabel 8550 1250 0    60   Input ~ 0
auger
Text GLabel 8550 1150 0    60   Input ~ 0
element
$Comp
L +5V #PWR13
U 1 1 58A43050
P 8450 900
F 0 "#PWR13" H 8450 750 50  0001 C CNN
F 1 "+5V" H 8450 1040 50  0000 C CNN
F 2 "" H 8450 900 50  0000 C CNN
F 3 "" H 8450 900 50  0000 C CNN
	1    8450 900 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR14
U 1 1 58A43083
P 8550 2700
F 0 "#PWR14" H 8550 2450 50  0001 C CNN
F 1 "GND" H 8550 2550 50  0000 C CNN
F 2 "" H 8550 2700 50  0000 C CNN
F 3 "" H 8550 2700 50  0000 C CNN
	1    8550 2700
	1    0    0    -1  
$EndComp
Wire Wire Line
	8600 1050 8450 1050
Wire Wire Line
	8450 1050 8450 900 
Wire Wire Line
	8550 1150 8600 1150
Wire Wire Line
	8550 1250 8600 1250
Wire Wire Line
	8550 1350 8600 1350
$Comp
L CONN_01X02 P5
U 1 1 58A431CA
P 9750 1200
F 0 "P5" H 9750 1350 50  0000 C CNN
F 1 "DZ_input" V 9850 1200 50  0000 C CNN
F 2 "" H 9750 1200 50  0000 C CNN
F 3 "" H 9750 1200 50  0000 C CNN
	1    9750 1200
	1    0    0    -1  
$EndComp
Text GLabel 9450 1250 0    60   Input ~ 0
dz_pin
Text GLabel 9500 1150 0    60   Input ~ 0
dz_supply
Wire Wire Line
	9500 1150 9550 1150
Wire Wire Line
	9450 1250 9550 1250
Text GLabel 1300 4500 0    60   Input ~ 0
fan_out
Text GLabel 1250 4200 0    60   Input ~ 0
ac_in
$Comp
L R R4
U 1 1 58A438A3
P 1850 4400
F 0 "R4" V 1930 4400 50  0000 C CNN
F 1 "220" V 1850 4400 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 1780 4400 50  0001 C CNN
F 3 "" H 1850 4400 50  0000 C CNN
	1    1850 4400
	0    1    1    0   
$EndComp
$Comp
L MOC3052M U2
U 1 1 58A439B7
P 3000 4500
F 0 "U2" H 2800 4700 50  0000 L CNN
F 1 "MOC3052M" H 3000 4700 50  0000 L CNN
F 2 "DIP-6" H 2800 4300 50  0000 L CIN
F 3 "" H 2975 4500 50  0000 L CNN
	1    3000 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 4400 2000 4400
Wire Wire Line
	1700 4400 1500 4400
Wire Wire Line
	1500 4400 1500 4500
Wire Wire Line
	1500 4500 1300 4500
$Comp
L GND #PWR4
U 1 1 58A43A87
P 2700 4800
F 0 "#PWR4" H 2700 4550 50  0001 C CNN
F 1 "GND" H 2700 4650 50  0000 C CNN
F 2 "" H 2700 4800 50  0000 C CNN
F 3 "" H 2700 4800 50  0000 C CNN
	1    2700 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 4800 2700 4600
$Comp
L R R5
U 1 1 58A43ADC
P 3800 4600
F 0 "R5" V 3880 4600 50  0000 C CNN
F 1 "1.5k" V 3800 4600 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 3730 4600 50  0001 C CNN
F 3 "" H 3800 4600 50  0000 C CNN
	1    3800 4600
	0    1    1    0   
$EndComp
$Comp
L Q_TRIAC_AAG D1
U 1 1 58A43BE6
P 4700 4500
F 0 "D1" H 4825 4525 50  0000 L CNN
F 1 "Q6015L5" H 4825 4450 50  0000 L CNN
F 2 "" V 4775 4525 50  0000 C CNN
F 3 "" V 4700 4500 50  0000 C CNN
	1    4700 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 4600 3950 4600
Wire Wire Line
	3650 4600 3300 4600
$Comp
L CONN_01X06 P4
U 1 1 58A43E71
P 8800 1300
F 0 "P4" H 8800 1650 50  0000 C CNN
F 1 "CONN_01X06" V 8900 1300 50  0000 C CNN
F 2 "" H 8800 1300 50  0000 C CNN
F 3 "" H 8800 1300 50  0000 C CNN
	1    8800 1300
	1    0    0    -1  
$EndComp
Wire Wire Line
	8600 1550 8550 1550
Wire Wire Line
	8550 1550 8550 2700
Wire Wire Line
	8600 1450 8550 1450
$Comp
L H11AA1 U3
U 1 1 58A44698
P 3700 4000
F 0 "U3" H 3500 4200 50  0000 L CNN
F 1 "H11AA1" H 3700 4200 50  0000 L CNN
F 2 "DIP-6" H 3500 3800 50  0000 L CIN
F 3 "" H 3700 4000 50  0000 L CNN
	1    3700 4000
	-1   0    0    1   
$EndComp
Wire Wire Line
	1250 4200 2950 4200
Wire Wire Line
	2950 4200 2950 4000
Wire Wire Line
	2950 4000 3400 4000
$Comp
L GND #PWR5
U 1 1 58A447E3
P 3050 3700
F 0 "#PWR5" H 3050 3450 50  0001 C CNN
F 1 "GND" H 3050 3550 50  0000 C CNN
F 2 "" H 3050 3700 50  0000 C CNN
F 3 "" H 3050 3700 50  0000 C CNN
	1    3050 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 3900 3400 3650
Wire Wire Line
	3400 3650 3050 3650
Wire Wire Line
	3050 3650 3050 3700
$Comp
L R R7
U 1 1 58A44A15
P 4400 3900
F 0 "R7" V 4480 3900 50  0000 C CNN
F 1 "15K" V 4400 3900 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4330 3900 50  0001 C CNN
F 3 "" H 4400 3900 50  0000 C CNN
	1    4400 3900
	0    1    1    0   
$EndComp
$Comp
L R R6
U 1 1 58A44BB3
P 4400 4100
F 0 "R6" V 4480 4100 50  0000 C CNN
F 1 "15K" V 4400 4100 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 4330 4100 50  0001 C CNN
F 3 "" H 4400 4100 50  0000 C CNN
	1    4400 4100
	0    1    1    0   
$EndComp
Wire Wire Line
	4000 4100 4250 4100
Wire Wire Line
	4700 4350 4700 4100
Wire Wire Line
	4550 4100 5450 4100
Connection ~ 4700 4100
$Comp
L GND #PWR6
U 1 1 58A44ECB
P 5650 4850
F 0 "#PWR6" H 5650 4600 50  0001 C CNN
F 1 "GND" H 5650 4700 50  0000 C CNN
F 2 "" H 5650 4850 50  0000 C CNN
F 3 "" H 5650 4850 50  0000 C CNN
	1    5650 4850
	1    0    0    -1  
$EndComp
Text Label 4850 4200 0    60   ~ 0
Live
$Comp
L CONN_01X06 P1
U 1 1 58A4538C
P 7750 950
F 0 "P1" H 7750 1300 50  0000 C CNN
F 1 "CONN_01X06" V 7850 950 50  0000 C CNN
F 2 "" H 7750 950 50  0000 C CNN
F 3 "" H 7750 950 50  0000 C CNN
	1    7750 950 
	1    0    0    -1  
$EndComp
Wire Wire Line
	7050 1200 7550 1200
$Comp
L +5V #PWR10
U 1 1 58A45858
P 7400 700
F 0 "#PWR10" H 7400 550 50  0001 C CNN
F 1 "+5V" H 7400 840 50  0000 C CNN
F 2 "" H 7400 700 50  0000 C CNN
F 3 "" H 7400 700 50  0000 C CNN
	1    7400 700 
	0    -1   -1   0   
$EndComp
$Comp
L +5V #PWR11
U 1 1 58A45903
P 7400 900
F 0 "#PWR11" H 7400 750 50  0001 C CNN
F 1 "+5V" H 7400 1040 50  0000 C CNN
F 2 "" H 7400 900 50  0000 C CNN
F 3 "" H 7400 900 50  0000 C CNN
	1    7400 900 
	0    -1   -1   0   
$EndComp
$Comp
L +5V #PWR12
U 1 1 58A4594A
P 7400 1100
F 0 "#PWR12" H 7400 950 50  0001 C CNN
F 1 "+5V" H 7400 1240 50  0000 C CNN
F 2 "" H 7400 1100 50  0000 C CNN
F 3 "" H 7400 1100 50  0000 C CNN
	1    7400 1100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7400 700  7550 700 
Wire Wire Line
	7400 900  7550 900 
Wire Wire Line
	7400 1100 7550 1100
Text Label 6650 800  0    60   ~ 0
Water
Text Label 6900 1000 0    60   ~ 0
Auger
Text Label 7100 1200 0    60   ~ 0
Light
$Comp
L +5V #PWR15
U 1 1 58A4B812
P 9850 2750
F 0 "#PWR15" H 9850 2600 50  0001 C CNN
F 1 "+5V" H 9850 2890 50  0000 C CNN
F 2 "" H 9850 2750 50  0000 C CNN
F 3 "" H 9850 2750 50  0000 C CNN
	1    9850 2750
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR16
U 1 1 58A4B879
P 9850 4200
F 0 "#PWR16" H 9850 3950 50  0001 C CNN
F 1 "GND" H 9850 4050 50  0000 C CNN
F 2 "" H 9850 4200 50  0000 C CNN
F 3 "" H 9850 4200 50  0000 C CNN
	1    9850 4200
	1    0    0    -1  
$EndComp
Text GLabel 9550 3150 0    60   Input ~ 0
SDA
Text GLabel 9550 3300 0    60   Input ~ 0
SCL
Wire Wire Line
	9550 3150 9700 3150
Wire Wire Line
	9700 3150 9700 3200
Wire Wire Line
	9700 3200 9950 3200
Wire Wire Line
	9550 3300 9950 3300
Wire Wire Line
	9950 3100 9850 3100
Wire Wire Line
	9850 3100 9850 2750
Text GLabel 1750 1900 0    60   Input ~ 0
SDA
Text GLabel 1800 2300 0    60   Input ~ 0
SCL
Wire Wire Line
	1750 1900 1900 1900
Wire Wire Line
	1800 2300 1900 2300
$Comp
L +5V #PWR2
U 1 1 58A4BC3F
P 2600 700
F 0 "#PWR2" H 2600 550 50  0001 C CNN
F 1 "+5V" H 2600 840 50  0000 C CNN
F 2 "" H 2600 700 50  0000 C CNN
F 3 "" H 2600 700 50  0000 C CNN
	1    2600 700 
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 850  2600 700 
Text GLabel 3550 1350 2    60   Input ~ 0
Button_1
Text GLabel 3550 1450 2    60   Input ~ 0
Button_2
Text GLabel 3550 1550 2    60   Input ~ 0
Button_3
Text GLabel 3550 1650 2    60   Input ~ 0
Button_4
$Comp
L CONN_01X08 P3
U 1 1 58A4BFF2
P 10150 3450
F 0 "P3" H 10150 3900 50  0000 C CNN
F 1 "LCD_Button" V 10250 3450 50  0000 C CNN
F 2 "" H 10150 3450 50  0000 C CNN
F 3 "" H 10150 3450 50  0000 C CNN
	1    10150 3450
	1    0    0    -1  
$EndComp
Text GLabel 9550 3450 0    60   Input ~ 0
Button_1
Text GLabel 9550 3600 0    60   Input ~ 0
Button_2
Text GLabel 9550 3750 0    60   Input ~ 0
Button_3
Text GLabel 9550 3900 0    60   Input ~ 0
Button_4
Wire Wire Line
	9550 3450 9600 3450
Wire Wire Line
	9600 3450 9600 3400
Wire Wire Line
	9600 3400 9950 3400
Wire Wire Line
	9550 3600 9650 3600
Wire Wire Line
	9650 3600 9650 3500
Wire Wire Line
	9650 3500 9950 3500
Wire Wire Line
	9550 3750 9750 3750
Wire Wire Line
	9750 3750 9750 3600
Wire Wire Line
	9750 3600 9950 3600
Wire Wire Line
	9550 3900 9850 3900
Wire Wire Line
	9850 3900 9850 3700
Wire Wire Line
	9850 3700 9950 3700
Wire Wire Line
	9850 4200 9950 4200
Wire Wire Line
	9950 4200 9950 3800
$Comp
L R R8
U 1 1 58A4C49D
P 1600 3950
F 0 "R8" V 1680 3950 50  0000 C CNN
F 1 "10k" V 1600 3950 50  0000 C CNN
F 2 "Resistors_ThroughHole:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal" V 1530 3950 50  0001 C CNN
F 3 "" H 1600 3950 50  0000 C CNN
	1    1600 3950
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR1
U 1 1 58A4C5A2
P 1600 3650
F 0 "#PWR1" H 1600 3500 50  0001 C CNN
F 1 "+5V" H 1600 3790 50  0000 C CNN
F 2 "" H 1600 3650 50  0000 C CNN
F 3 "" H 1600 3650 50  0000 C CNN
	1    1600 3650
	1    0    0    -1  
$EndComp
Wire Wire Line
	1600 3800 1600 3650
Wire Wire Line
	1600 4100 1600 4200
Connection ~ 1600 4200
$Comp
L CONN_01X08 P6
U 1 1 58A4C957
P 5900 4150
F 0 "P6" H 5900 4600 50  0000 C CNN
F 1 "240_CONN_Therm_on_3_4" V 6000 4150 50  0000 C CNN
F 2 "" H 5900 4150 50  0000 C CNN
F 3 "" H 5900 4150 50  0000 C CNN
F 4 "Thermo connects to 3 and 4" H 5900 4150 60  0001 C CNN "Description"
	1    5900 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	4700 4650 5450 4650
Wire Wire Line
	5450 4650 5450 4400
Wire Wire Line
	5450 4400 5700 4400
Text Label 5200 4650 0    60   ~ 0
Fan_out
Wire Wire Line
	5700 4500 5650 4500
Wire Wire Line
	5650 4500 5650 4850
Wire Wire Line
	5450 4100 5450 4300
Wire Wire Line
	5450 4300 5700 4300
Wire Wire Line
	4000 3900 4250 3900
Wire Wire Line
	4550 3900 5550 3900
Wire Wire Line
	5550 3900 5550 4200
Wire Wire Line
	5550 4200 5700 4200
Text Label 5000 3900 0    60   ~ 0
Neutral
Wire Wire Line
	5700 4100 5550 4100
Connection ~ 5550 4100
Wire Wire Line
	5700 3800 5700 4000
Connection ~ 5700 3900
Text Label 5700 3850 0    60   ~ 0
N
Wire Wire Line
	4050 1950 3300 1950
Text GLabel 3450 2600 2    60   Input ~ 0
ac_in
Wire Wire Line
	3450 2600 3300 2600
Wire Wire Line
	3550 1650 3300 1650
Wire Wire Line
	3550 1550 3300 1550
Wire Wire Line
	3550 1450 3300 1450
Wire Wire Line
	3550 1350 3300 1350
$EndSCHEMATC
