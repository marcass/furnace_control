#include <math.h> 
const int PUMP = 7;     
const int WATER_TEMP = 3;     
int water_temp;

 
//Analogue reading maths for temp
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
  pinMode(PUMP, OUTPUT);      
  digitalWrite(PUMP, HIGH); 
  Serial.begin(115200);  
}

void loop(){
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  Serial.print("water tempm = ");
  Serial.println(water_temp);
}
