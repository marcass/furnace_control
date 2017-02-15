#include <math.h>

/* 5V --- sensor --- 10k ohm --- gnd
                  |
                  |
                 A0/1
*/

int photocellPin = 1;     // the cell and 10K pulldown are connected to a1
int photocellReading;     // the analog reading from the sensor divider

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
 Serial.begin(115200);
}

void loop() {
 Serial.print("Temperature reading = ");
 Serial.println(int(Thermistor(analogRead(0))));  // display Fahrenheit
 photocellReading = analogRead(photocellPin);  
 Serial.print("Light reading = ");
 Serial.println(photocellReading);     // the raw analog reading
 delay(100);
}
