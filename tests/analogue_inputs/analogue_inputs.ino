#include <math.h>

//Inputs
const int WATER_TEMP = 3; //analogue pin 3
const int AUGER_TEMP = 6; //analogue pin 6
const int LIGHT = 2; //analogue pin 2 for flame detection

//variables
int auger_temp;
int water_temp;
int flame_val;

//Analogue reading maths
double Thermistor(int RawADC) {
 double Temp;
 Temp = log(10000.0*((1024.0/RawADC-1)));
 //         =log(10000.0/(1024.0/RawADC-1)) // for pull-up configuration
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;            // Convert Kelvin to Celcius
 //Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit
 return Temp;
}

void setup() {
  // initialize serial communication:
  Serial.begin(115200);
}

void loop() {
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  flame_val = analogRead(LIGHT);
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  Serial.print("Auger = ");
  Serial.print(auger_temp);
  Serial.print("  Flame = ");
  Serial.print(flame_val);
  Serial.print("  Water = ");
  Serial.println(water_temp);
  delay(1000);
}

