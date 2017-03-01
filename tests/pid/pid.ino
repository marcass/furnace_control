
#include <math.h>
#include <PID_v1.h>


//PID setup
//double Setpoint, Input, Output;
double TEMP_SET_POINT = 68; //PID SETPOINT
double power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
double water_temp;
double feed_pause_percent;
double FEED_SET_POINT = 20000; //20s in ms
int feed_pause = 2000;

//Specify the links and initial tuning parameters
//PID fanPID(&Input, &Output, &Setpoint,2,5,1, DIRECT);
PID fanPID(&water_temp, &power, &TEMP_SET_POINT,2,5,1, DIRECT); //need more fan power to get hotter so DIRECT
PID pelletsPID(&water_temp, &feed_pause_percent, &FEED_SET_POINT,2,5,1, REVERSE); //need shorter feed time to get to hotter so REVERSE


//Inputs
const int WATER_TEMP = 3; //analogue pin 3

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
  // initialize serial communication:
  Serial.begin(115200);
  //initialize the PID variables we're linked to
  fanPID.SetOutputLimits(30, 80); //percentage of fan power
  fanPID.SetSampleTime(500); //SAMPLES EVERY 0.5s
  pelletsPID.SetOutputLimits(60,100); //percentage of feed time check to see that burning all of load
  pelletsPID.SetSampleTime(500);
  //turn the PID on
  fanPID.SetMode(AUTOMATIC);
  pelletsPID.SetMode(AUTOMATIC);
}

void loop() {
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  fanPID.Compute();
  pelletsPID.Compute();
  feed_pause = (feed_pause_percent / 100) * 20000; //caluclate actual pause from PID derived value
  Serial.print("Fan power = ");
  Serial.print(power);
  Serial.print("%");
  Serial.print("  Pellets pause = ");
  Serial.print(feed_pause_percent);
  Serial.print("%");
  Serial.print("  Pellets feed pause time = ");
  Serial.print((int)feed_pause);
  Serial.println("ms");
}
