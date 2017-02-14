/*******************************
 BOILER CONTROL
 *******************************/
#define debug

//Libraries
//EEPROM for saving points const ints
//PID library
//Liquid crystal display?

//Constants
const int ELEMENT_TIME = 120000; //2min in ms
const int START_FEED_TIME = 30000; //30s in ms for pellet feed initially
const int LOW_TEMP = 50; //deg C -> low end of heating range
const int HIGH_TEMP = 85; //deg C -> high end of heating range
const int OVER_TEMP = 90; //deg C = Oh shit lets shut down
const int START_FAN_TIME = 20000; //20s in ms for time to blow to see if flame present
const int FLAME_VAL_THRESHOLD;//work out a value here that is reasonable
//States
const int STATE_IDLE = 0;
const int STATE_START_UP = 1;
const int STATE_HEATING = 2;
const int STATE_COOL_DOWN = 3;
const int STATE_ERROR = 4;

//Variables that change
unsigned long element_start = 0;
unsigned long fan_start = 0;
int water_temp;
int auger_temp;
int flame_val;
int state = 0;

//Outputs
const int PUMP = 2; //circ pump relay
const int FAN = 9; //fan relay. pin9 is a PWM pin and allows for analogWrite
const int AUGER = 3; //pellet auger relay
const int ELEMENT = 4; //fire starting element relay

//Inputs
const int WATER_TEMP = 3; //analogue pin 3
const int AUGER_TEMP = 4; //analogue pin 4
const int LIGHT = 2; //analogue pin 2 for flame detection
const int DZ_PIN = 5; // pin that is pulled up when 1-wire relay closes
const int DZ_SUPPLY = 6; //Need a pin to supply 5v that is passed to DZ_PIN when 1-wire relay closes



void setup() {
  //intitialise output pins (inputs are all analogue read pins)
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, LOW); //pull down with a 10k or is there internal pulldown?
  pinMode(FAN, OUTPUT);
  digitalWrite(FAN, LOW); //pull down with a 10k or is there internal pulldown?
  pinMode(AUGER, OUTPUT);
  digitalWrite(AUGER, LOW); //pull down with a 10k or is there internal pulldown?
  pinMode(ELEMENT, OUTPUT);
  digitalWrite(ELEMENT, LOW); //pull down with a 10k or is there internal pulldown?
  pinMode(DZ_SUPPLY, OUTPUT);
  digitalWrite(DZ_SUPPLY, HIGH); //pull up?
  //Initialise inputs
  pinMode(DZ_PIN, INPUT);
  digitalWrite(DZ_PIN, LOW); //pull down with a 10k or is there internal pulldown?
  // don't think analogue pins need to be initialised pinMode(WATER_TEMP, INPUT);
  
  // initialize serial communication:
  Serial.begin(115200);
}




void loop() {
  manage_outputs();
  switch (state) {
    case STATE_IDLE:
      proc_idle();
      break;
    case STATE_START_UP:
      proc_start_up();
      break;
    case STATE_HEATING:
      proc_heating();
      break;
    case STATE_COOL_DOWN:
      proc_cool_down();
      break;
    case STATE_ERROR:
      proc_error();
      break;
  }
}
