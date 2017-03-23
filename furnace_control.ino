/* ToDo
 *  - check lcd working
 *  - put ultrasound probe on analoge pins (as digital?)
 */

/*******************************
 BOILER CONTROL
 *******************************/
//#define debug
#define pid //use if PID controlling fan output
//#define no_PID //use if not pid controlling
#define mqtt
//#define ac_counter
//#define lcd //interferes with interrupts?

//Libraries
//#include <Wire.h>
//#ifdef lcd
//  //https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
//  #include <LiquidCrystal_I2C.h>
//  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
////need to set contrast and backlight and check pin assignments
//#endif
//PID library
#include <math.h>
#include <PID_v1.h>
//for ac phase angle stuff
#include <avr/io.h>
#include <avr/interrupt.h>
#include "FastRunningMedian.h" //for getting flame val median http://forum.arduino.cc/index.php?topic=53081.msg1160999#msg1160999

/*******************************************************************************
 * Constants
 */
//timers
const long ELEMENT_TIME = 450000; //7.5min in ms
const long START_FEED_TIME = 120000; //2min in ms for pellet feed initially
const long SUBSEQUENT_START_FEED_TIME = 5000; //little top up of pellets if not starting first time
const long START_FAN_TIME = 90000; //90s in ms for time to blow to see if flame present
//const long DUMP_START = 45000;//45s of fanning before throwing a little fuel on the fire
const long END_FAN_TIME = 360000; //6min of blow to empty puck from burn box
const long PUMP_TIME = 30000; //30s in ms to avoid short cycling pump
const int BUTTON_ON_THRESHOLD = 1500;//1.5s in ms for turning from off to idle and vice versa
const long FEED_PAUSE = 24000; //60s and calculating a result so might need to be a float
const long FEED_TIME = 10000;//default pellet feed time
const long STATE_CHANGE_THRES = 5000;//time that needs to elapse before changing STATE_START_UP <- STATE_HEATING
const long STATE_CHANGE_THRES_UP = 2000;//time that needs to elapse before changing STATE_START_UP -> STATE_HEATING
const long STOP_THRESH = 5000; //for  fan short cycling
const long ERROR_THRES = 5000;//time that must elapse before goes into error at startup (not working at present)
const long RESET_THRESHOLD = 300000; //time that must elapse before start count gets reset
const int FLAME_READ_INTERVAL = 30; //read flame val every 30ms to build array for calculating average value (smoothing)
//temps
const int LOW_TEMP = 50; //deg C -> low end of heating range
const int HIGH_TEMP = 75; //deg C -> high end of heating range
const int MID_TEMP = 63;//Send back to heating if over heats from here
const int TOO_HOT_TEMP = 85; //cool down NOW
const int AUGER_OVER_TEMP = 55; //deg C - don't want a hopper fire
//flame values
const int FLAME_VAL_THRESHOLD = 120;//work out a value here that is reasonable
const int START_FLAME = 50;
//States
const int STATE_IDLE = 0;
const int STATE_START_UP = 1;
const int STATE_HEATING = 2;
const int STATE_COOL_DOWN = 3;
const int STATE_ERROR = 4;
const int STATE_OFF = 5;
//Buttons (numbers from left
const int BUTTON_1 = 13;
const int BUTTON_2 = 12;
const int BUTTON_3 = 11;
const int BUTTON_4 = 10;
//Outputs
const int PUMP = 7; //circ pump relay
const int FAN = 9; //fan relay. pin9 is a PWM pin and allows for analogWrite
const int AUGER = 3; //pellet auger relay
const int ELEMENT = 4; //fire starting element relay
//Inputs
const int WATER_TEMP = 3; //analogue pin 3
const int AUGER_TEMP = 6; //analogue pin 6
const int LIGHT = 2; //analogue pin 2 for flame detection
const int DZ_PIN = 5; // pin that is pulled up when 1-wire relay closes
const int DZ_SUPPLY = 6; //Need a pin to supply 5v that is passed to DZ_PIN when 1-wire relay closes
#define DETECT 2 //ac detection pin
#define GATE 9 //pwm pin
#define PULSE 4   //trigger pulse width (counts) that triac requires to fire one specified needs ~25mus

/********************************************************************************************
 * Variables
 */ 
// general timers (specific ones in subsection)
unsigned long debounce_start = 0; //button press debounce
unsigned long element_start = 0;
unsigned long fan_start = 0;
unsigned long auger_start = 0;
unsigned long reset_dump_count;
unsigned long state_trans_start = 0;
unsigned long state_trans_stop = 0;
unsigned long stop_start = 0; //for  fan short cycling
unsigned long start_feed_time = 0;//pellets
unsigned long start_feed_pause = 0;//pellets
unsigned long start_pump_time = 0;//pump short cycling
int on_wait; //variable for converting power to timer value
unsigned long feed_pause;//var for pellets puase time
unsigned long feed_time;//var for feed time
//sensor readings
int auger_temp;
int flame_val; // range from 0 to 1024 ( i think)
//counters
int start_count = 0;
int crosses;
int counts;
int dump_count = 0;//counter to make sure we don't fill up chamber with pellets
//tests
bool reset = false;
bool feeding = true;
bool cooling = false;
bool dump = false;
bool elem = false;
bool blowing = false;
//maths
int proportion;
float divisor;
//state stuff
String reason = "";
int state;
int this_state;
//reading flame value stuff and smoothing
FastRunningMedian<int, 32, 1> newMedian;
// Constructor: 
// FastRunningMedian<datatype_of_content, size_of_sliding_window, default_value> 
// maximim size_of_sliding_window is 255
// Methods:
// addValue(val) adds a new value to the buffers (and kicks the oldest)
// getMedian() returns the current median value
unsigned long prevMillis;
//const int numReadings = 10;
//int readings[numReadings];      // the readings from the light sensor
//int readIndex = 0;              // the index of the current reading
//int total = 0;

/****************************************************************************************
 * PID
 */
#ifdef pid
  double Kp=10, Ki=0.001, Kd=1; //P, I and D see http://playground.arduino.cc/Code/PIDLibrarySetTunings
  double temp_set_point = 63; //PID temp SETPOINT
  double power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
  double water_temp;
  double feed_pause_percent;
  double feed_percent;
  //PID fanPID(&Input, &Output, &Setpoint,2,5,1, DIRECT);
  PID fanPID(&water_temp, &power, &temp_set_point,Kp, Ki, Kd, DIRECT); //need more fan power to get hotter so DIRECT
  PID pausePID(&water_temp, &feed_pause_percent, &temp_set_point,Kp, Ki, Kd, REVERSE); //need shorter feed time to get to hotter so REVERSE
  PID feedPID(&water_temp, &feed_percent, &temp_set_point,Kp, Ki, Kd, DIRECT);
  /* maths  may need to be addedfor value calcs (pid outputs value of 0-255 and we want to convert to 
   *  range in percentage limits
   *  http://playground.arduino.cc/Code/PIDLibrarySetOutputLimits states that 
   *  feedPID.SetOutputLimits(40,100); should alter default behavior, but I wonder if it is a trncater version
   *  of 0-255
   *  - fan runs at 30-80% of power (phase angle control fails outside that range)
   *  - pellets feed I want to run at 40-100% of time in range
   *  - pause I want to run between 60-100% of time in range
   */
#endif

#ifdef no_PID
  int power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
  int water_temp; 
#endif

//MQTT *****************************************************************************************

#ifdef mqtt
  long PUB_INTERVAL = 1000; //1 second to cylce between publishing events while running
  long previousMillis = 0;
  const long PUB_INTERVAL_IDLE = 30000; //publish state info and temp info for logging while not running
  int index = 0; //counter for cycling through publishing array
  //topics
  int fan_power; //what fan is actually running at
  String STATE_TOPIC = "boiler/state";
  String WATER_TEMP_TOPIC = "boiler/temp/water";
  String AUGER_TEMP_TOPIC = "boiler/temp/auger";
  String FLAME_TOPIC = "boiler/flame";
  String ERROR_TOPIC = "boiler/messages";
  String PID_FAN = "boiler/pid/fan";
  String PID_FEED = "boiler/pid/feed";
  String PID_PAUSE = "boiler/pid/pause";
  String START_COUNT_TOP = "boiler/start_count";
  String TEMP_SET_POINT_TOP = "boiler/temp/setpoint";
  const String STATES_STRING[] = {"Idle","Starting","Heating","Cool down","Error","Off","Wind down"};
  
  //publish functions overloaded for int/string as payload
  void publish(String top,int payload) { //publish data
    Serial.print("MQTT:");
    Serial.print(top);
    Serial.print("/");
    Serial.println(payload);
  }
  void publish(String top,String payload) { //publish data
    Serial.print("MQTT:");
    Serial.print(top);
    Serial.print("/");
    Serial.println(payload);
  } 
#endif

// SETUP SERIAL COMM for inputs *********************************************************
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();  // add it to the inputString:
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}

//phase angle interrupts ***************************************************************
void zeroCrossingInterrupt(){ //zero cross detect   
  TCCR1B=0x04; //start timer with divide by 256 input
  TCNT1 = 0;   //reset timer - count from zero
  crosses++;
}

ISR(TIMER1_COMPA_vect){ //comparator match
  digitalWrite(GATE,HIGH);  //set TRIAC gate to high
  TCNT1 = 65536-PULSE;      //trigger pulse width
}

ISR(TIMER1_OVF_vect){ //timer1 overflow
  digitalWrite(GATE,LOW); //turn off TRIAC gate
  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

//Analogue reading maths for temp ******************************************************
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
  //intitialise output pins (inputs are all analogue read pins)
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, HIGH); //enable pull-up resistor
  pinMode(GATE, OUTPUT);      //TRIAC gate control
  digitalWrite(GATE, LOW);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, LOW);
  pinMode(FAN, OUTPUT);
  digitalWrite(FAN, LOW);
  pinMode(AUGER, OUTPUT);
  digitalWrite(AUGER, LOW);
  pinMode(ELEMENT, OUTPUT);
  digitalWrite(ELEMENT, LOW);
  pinMode(DZ_SUPPLY, OUTPUT);
  digitalWrite(DZ_SUPPLY, LOW);
  //digitalWrite(A2, LOW);
  //Initialise inputs
  pinMode(DZ_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  #ifdef pid
    //initialize the PID variables we're linked to
    fanPID.SetOutputLimits(30, 80); //percentage of fan power
    fanPID.SetSampleTime(3000); //SAMPLES EVERY 3s
    pausePID.SetOutputLimits(100, 170); //percentage of feed time check to see that burning all of load
    pausePID.SetSampleTime(3000);
    feedPID.SetOutputLimits(60,100); //percentage of feed time check to see that burning all of load
    feedPID.SetSampleTime(3000);
    //turn the PID on
    fanPID.SetMode(AUTOMATIC);
    pausePID.SetMode(AUTOMATIC);
    feedPID.SetMode(AUTOMATIC);
  #endif
  // set up Timer1 
  //(see ATMEGA 328 data sheet pg 134 for more details)
  OCR1A = 100;      //initialize the comparator
  TIMSK1 = 0x03;    //enable comparator A and overflow interrupts
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled
  // set up zero crossing interrupt
  attachInterrupt(0,zeroCrossingInterrupt, RISING); //IRQ0 is pin 2. Call zeroCrossingInterrupt 
  // initialize serial communication:
  Serial.begin(9600);//slower speed for long cable
  inputString.reserve(200); // reserve mem for received message on serial port 
  int state = 0;
  #ifdef mqtt
    //initialise state as idle on statup
    Serial.print("MQTT:");
    Serial.print(STATE_TOPIC);
    Serial.print("/");
    Serial.println(STATES_STRING[state]);
  #endif
}

//Functions that do stuff ********************************************************************
void fan(bool runFan, int x) {
  #ifdef mqtt
    fan_power = x;
  #endif
  if (!runFan) {
    digitalWrite(GATE,LOW);
    //undo phase angle magic here 
    TCCR1B = 0x00;
    TIMSK1 = 0x00;    //disable comparator A and overflow interrupts
    TCCR1A = 0x00;    //timer control registers set for
    TCCR1B = 0x00;    //normal operation, timer disabled
    stop_start = 0;
    power = 0;
  }
  if (runFan) {
    #ifdef debug
      Serial.print(" FAN ON at ");
      Serial.print(x);
    #endif
    if (x == 100) { //no phase angle control needed if you want balls out fan speed
      digitalWrite(GATE,HIGH);
    }else {
      //do magic phase angle stuff here
      /* x is the int as a percentage of fan power
       * OCR1A is the comparator for the phase angle cut-off
       * - when TCNT1 > OCR1A, ISR(TIMER1_OVF_vect) is called tellng optocoupler to power down
       * - 520 counts (16000000 cycles scaled to 256) per half AC sine wave @60hz (i think we run at 50Hz)
       * - fan stops over 80% of power, presumably latching the triac confusion over next zero cross => less than 156 count delay
       * - 30% is the lower power limit to avoid firing the triac too close to teh zero cross => delay not more than 364 count delay
       * - The smaller the value of OCR1A the more power we have
       */
      TIMSK1 = 0x03;    //enable comparator A and overflow interrupts
      //set up interrupt
      
      //attachInterrupt(0,zeroCrossingInterrupt, RISING);  // inturrupt 0 on digital pin 2
      
      //set a value that is a proportion of 520 for power
      divisor = (float)x / 100;
      proportion = divisor * 520;
      on_wait = 520 - proportion;
      if ( on_wait < 104) {
        OCR1A = 104;
      }
      if ( on_wait > 364) {
        OCR1A = 364;
      }else {
        OCR1A = on_wait;
      }
    }
  }
}

void pump(bool on) { //prevents short cycling of pump
  #ifdef debug
    Serial.print(" Pump: ");
    Serial.print(on);
  #endif 
  if (on) {
    digitalWrite(PUMP, HIGH);
    if (start_pump_time == 0) {    //avoid short cycling so pump for at least 30s
      start_pump_time = millis();
    }
  }
  if (!on) {
    if ((long)(millis() - start_pump_time) > PUMP_TIME) {
      digitalWrite(PUMP, LOW);
      start_pump_time = 0;
    }
  }
}

void flame_value_median() {
  unsigned long currentMillis = millis();
  if(currentMillis - prevMillis > FLAME_READ_INTERVAL) { 
    prevMillis = currentMillis; 
    newMedian.addValue(analogRead(LIGHT));
    flame_val = newMedian.getMedian();
  }
}


void fan_and_pellet_management() {
  //water temp measured in safety()
  //FAN MANAGEMENT
  if (water_temp < LOW_TEMP) { //go hard on the fan
    fan(true, 100);
  }else if (water_temp > HIGH_TEMP) {
    this_state = STATE_HEATING;
    state = STATE_COOL_DOWN;
    #ifdef mqtt
      //
      publish(STATE_TOPIC, STATES_STRING[state]);
    #endif    
  }else {
    #ifdef no_PID
      power = 75; //arbitrary value
    #endif
    fan(true, (int)power);
  }
  //PELLETS MANAGEMENT
  #ifdef pid
    feed_pause = (feed_pause_percent / 100) * FEED_PAUSE; //caluclate actual pause from PID derived value
    feed_time =  (feed_percent / 100)       * FEED_TIME;
  #endif
  #ifdef no_PID //won't be changing so set to const int value
    feed_time = FEED_TIME;
    feed_pause = FEED_PAUSE;
  #endif
  if (feeding) {
    if (start_feed_time == 0) {
      start_feed_time = millis();
    }
    //test to see if feed been on for long enough
    if ((long)(millis() - start_feed_time) > feed_time) {
      //stop feeding and start pausing
      feeding = false;
      start_feed_time = 0;
      //start_feed_pause = millis();
      digitalWrite(AUGER, LOW);
      #ifdef debug
        Serial.print("  Auger off  ");
      #endif
    }else {
      digitalWrite(AUGER, HIGH);
      #ifdef debug
        Serial.print("  Auger on  ");
      #endif 
    }
  } 
  if (!feeding) { 
    if (start_feed_pause == 0 ) {
      start_feed_pause = millis();
    }
    if ((long)(millis() - start_feed_pause) > feed_pause) {
      //stop pausing, start feeding
      
      start_feed_pause = 0;
      feeding = true;
    }
  }
//  #ifdef debug
//    Serial.print(" Time left to feed = ");
//    Serial.print(feed_time - (millis()-start_feed_time));
//    Serial.print("  Time left in pause = ");
//    Serial.println(feed_pause - (millis()-start_feed_pause));
////      Serial.print("  Feed pause percent = ");
////      Serial.print(feed_pause_percent);
//  #endif
    
}

void going_yet() {
  /*startup sequence
   * 1. check to see if heaps of light -> heating
   * 2. check to see if a little bit of light -> fan and go to 1
   * 3. if no light -> pellet dump and element start then go to 1
   */
  //read light in firebox (done in loop)
  if (flame_val > START_FLAME) { //If a bit of light get to heating
    if (state_trans_start == 0) { //smooth state transitions
      state_trans_start = millis();
    }
    if ((long)(millis() - state_trans_start) > STATE_CHANGE_THRES_UP) {
      digitalWrite(AUGER, LOW);
      digitalWrite(ELEMENT, LOW);
      blowing = false;
      state = STATE_HEATING;
      start_count = 0;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif    
      fan_start = 0;
      element_start = 0;
      auger_start = 0;
      reset = true; //watch for timer to reset dump_count
      reset_dump_count = millis();
      state_trans_start = 0;
      dump = false;
    } //else keep coming back to check
  }else {
     state_trans_start = 0; //not enough flame for state trans so reset timer
  }
}

void cool_to_stop(int target_state) {
  #ifdef debug
    Serial.print(" CoolToStop target = ");
    Serial.print(target_state);
  #endif
  //make sure stuff stays off
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  if (water_temp < (temp_set_point + 5)) { //5 seems a reasonable hysteresis value
    fan(true, 100);
  }
//  #ifdef debug
//    Serial.print("  Cooling down as all off  ");
//  #endif 
  //if fire continue fanning
  if (flame_val > START_FLAME) {
      fan_start = 0; //still have light so reset final blow
    }
  //Keep pumping heat into house until boiler cool
  if (water_temp < (temp_set_point + 2)){
    pump(false);
//    #ifdef debug
//      Serial.print(" Pump off ");
//    #endif
    if (fan_start == 0) {
      fan_start = millis();
    }
    if ((long)(millis() - fan_start) > END_FAN_TIME) { 
      fan(false, 0); //puck blown to peices, clean grate for next light
      state = target_state;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif   
      fan_start = 0;
    } 
  }else {
    //start pump to dump heat
    pump(true);
    fan_start = 0;//reset fan timer 
//    #ifdef debug
//      Serial.print("  Pump on  ");
//    #endif  
  }
}

void housekeeping() {
  //if (runFan) {
  fan(false, 0);
    //stop_fan();
  //}
  if (digitalRead(PUMP) == HIGH) {
    digitalWrite(PUMP, LOW);
  }
  if (digitalRead(ELEMENT) == HIGH) {
    digitalWrite(ELEMENT, LOW);
  }
  if (digitalRead(AUGER) == HIGH) {
    digitalWrite(AUGER, LOW);
  }
  if (dump) {
    dump = false;
  }
  if (elem) {
    elem = false;
  }
  if (element_start != 0) {
    element_start = 0;
  }
  if (fan_start != 0) {
    fan_start = 0;
  }
  if (auger_start != 0) {
    auger_start = 0;
  }
  if (reset_dump_count != 0) {
    reset_dump_count = 0;
  }
  if (dump_count > 0) {
    dump_count = 0;
  }
  if (state_trans_start != 0) {
    state_trans_start = 0;
  }
  if (state_trans_stop != 0) {
    state_trans_stop = 0;
  }
  if (stop_start != 0) {
    stop_start = 0;
  }
  if (start_count != 0) {
    start_count = 0;
  }
}

//blow fan for a time
void blow() {
  fan(true, 100);
  if (fan_start == 0) {
    fan_start = millis();
    blowing = true;
  }
  if ((long)(millis() - fan_start) > START_FAN_TIME) {
    fan(false, 0);
    fan_start = 0;
    blowing = false;
  }
}


void safety() {
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > TOO_HOT_TEMP) {
    this_state = STATE_ERROR;
    state = STATE_COOL_DOWN;
    reason = "Too hot";
    #ifdef mqtt
      //
      publish(STATE_TOPIC, STATES_STRING[state]);
    #endif    
  }
  //if auger too hot go into error
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
    #ifdef mqtt
      //
      publish(ERROR_TOPIC, reason);
      publish(STATE_TOPIC, STATES_STRING[state]);
      reason = "";
    #endif    
  }
}

//state fucntions *****************************************************************
void proc_idle() {
  housekeeping();
  //1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == LOW) {
    state = STATE_START_UP;
    reason = "";
    #ifdef mqtt
      //
      publish(STATE_TOPIC, STATES_STRING[state]);
      publish(ERROR_TOPIC, reason); //clear error register
    #endif
  }
}

void proc_start_up() {
  //kill if failed to start too many times
  if (start_count > 2) {
    state = STATE_ERROR;
    reason = "failed to start";
    #ifdef mqtt
      //
      publish(ERROR_TOPIC, reason);
      reason = "";
      dump = false;
    #endif
   }
  #ifdef debug
    Serial.print("  Dump =   ");
    Serial.print(dump);
    Serial.print("  DumpCount =  ");
    Serial.print(dump_count);
  #endif
  going_yet(); //perform check in each loop
  if (start_count == 0) {
   blow();
   if (!blowing) {
      start_count++; //increment out of this loop
      dump = true;
    }
  }
  if (start_count > 0) {
    if (dump) {
      fan(false, 0);
      if (auger_start == 0) {
        auger_start = millis();
      }
      if (dump_count == 0) {
        if ((long)(millis() - auger_start) > START_FEED_TIME) {
          //stop feeding pellets
          digitalWrite(AUGER, LOW);
          auger_start = 0;
          dump = false;
          dump_count++;
          elem = true;
          #ifdef debug
            Serial.print("  Auger off  ");
          #endif
        }
      }
      if (dump_count > 0) { //don't want to overload pellet chamber
        if ((long)(millis() - auger_start) > SUBSEQUENT_START_FEED_TIME) {
        //stop feeding pellets
        digitalWrite(AUGER, LOW);
        auger_start = 0;
        dump_count++;
        dump = false;
        elem = true;
        #ifdef debug
          Serial.print("  Auger off  ");
        #endif
        }
      }
      digitalWrite(AUGER, HIGH); //dump pellets
      digitalWrite(ELEMENT, LOW);
      #ifdef debug
        Serial.print(" Auger on ");
      #endif  
      
    }     
    if (elem) {
      if (element_start == 0) { //start element timer if not already started
        element_start = millis();
      }
      //test to see if element been on for too enough and stop it if it has
      if ((long)(millis() - element_start) > ELEMENT_TIME) {
        digitalWrite(ELEMENT, LOW);
        blow();
        if (!blowing) {
          element_start = 0;
          start_count++;
          elem = false;
          dump = true;
        }
      }else {
        digitalWrite(ELEMENT, HIGH); //start element
        fan(false, 0);
        digitalWrite(AUGER, LOW);
        #ifdef debug
          Serial.print("  Element on  ");
        #endif
      }     
    }
  }
  #ifdef debug
    Serial.print("Start count = ");
    Serial.print(start_count);
  #endif   
  if (water_temp > LOW_TEMP) {
    pump(true);
  }else {
    pump(false);
  }
}

void proc_heating() {
//  if (dump_count != 0) {//can reset dump count if heating starts
//    dump_count = 0;
//  }
  //do i want to have an intial hot run ot burn pellet load from start?
  #ifdef pid
    //set fan power and pellets pausse variable via PID lib here
    fanPID.Compute();
    pausePID.Compute();
    feedPID.Compute();
//    #ifdef debug
//      Serial.print("Fan power = ");
//      Serial.print(power);
//      Serial.print("%");
//      Serial.print("  Feed time = ");
//      Serial.print(feed_percent);
//      Serial.print("%");
//      Serial.print("  Pellets pause = ");
//      Serial.print(feed_pause_percent);
//      Serial.print("%  ");
//    #endif
  #endif
  //If not enough flame start again
  if (flame_val < FLAME_VAL_THRESHOLD) {
    if (state_trans_start == 0) { //smooth state transitions
      state_trans_start = millis();
    }
    if ((long)(millis() - state_trans_start) > STATE_CHANGE_THRES) {
      state = STATE_START_UP;
      dump = false;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif    
      state_trans_start = 0;
    }else { //start a counter to reset state_trans_start
      if (state_trans_stop == 0) {
        state_trans_stop = millis();
      }
      if ((long)(millis() - state_trans_stop) > STATE_CHANGE_THRES) {
        state_trans_start = 0;
      }
    }
  }
  fan_and_pellet_management();
  //pump water when it is in the bands
  if (water_temp > LOW_TEMP) {
    //start pump
    pump(true);
    #ifdef debug
      Serial.print(" Pump on ");
    #endif    
  }
  if (water_temp < LOW_TEMP) {
    pump(false);
  }
}

void proc_cool_down(int pts) {
  /*order goes like this:
   * 1. Too hot? -> pump to get below mid range
   * 2. send to heating if heating sent to cool down
   * 3. send to error/off/idle if they sent to cool down
   */
  #ifdef debug
    Serial.print(" Passthrough state = ");
    Serial.print(pts);
  #endif
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  if (water_temp > MID_TEMP) {
    pump(true);
  }else {
    pump(false);
  }
  if ((water_temp < 70) and (water_temp > (temp_set_point + 7))) {
    fan(false, 0);
  }
  if (water_temp > (HIGH_TEMP - 1)) {
    //do nothing except pump
//    #ifdef debug
//      Serial.println("  Fan, auger and element off, pump on TOO HOT  ");
//    #endif
  }else if (water_temp > (temp_set_point - 5)) {
    if (pts != STATE_HEATING) { // not heating so cool down to stop
      cool_to_stop(pts); //this function should trigger state change above temp_set_point - 5
    }else if (water_temp < (temp_set_point + 1)) { //go back heating once enough heat has been dumped
      state = STATE_HEATING;
    }
  }else {
    cool_to_stop(pts);  
  }
}

void proc_error() {
  #ifdef debug
    Serial.print("Everything off except pump maybe,  ");
    Serial.print(reason);
  #endif
  //test if boiler too hot, if it is pump some water to cool it
  if (water_temp > MID_TEMP) {
    this_state = STATE_ERROR;
    state = STATE_COOL_DOWN;
  }else {
    housekeeping();//turn everything off and keep checking it is off
  }
  //error reason gets published in loop()
  //turn off if serial comms received
  if (stringComplete) {
    if (inputString.startsWith("Turn Off Boiler")) {
      inputString = "";
      stringComplete = false;
      state = STATE_OFF;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif      
      //delay(10);
    }else {
      reason = inputString;
      #ifdef mqtt
        publish(ERROR_TOPIC, reason); //dump message so we can see how it was malformed
      #endif
      reason = "";
      inputString = "";
      stringComplete = false;  
    }
  }
}

void proc_off() {
  //reset some shit
  start_count = 0;
  start_feed_time = 0;
  start_feed_pause = 0;
  start_pump_time = 0;
  debounce_start = 0;
  reason = "";
  //if too hot pump unitl not:
  if ((water_temp > MID_TEMP) || (flame_val > FLAME_VAL_THRESHOLD)) {
    this_state = state;
    state = STATE_COOL_DOWN;   
  }else {
    housekeeping();//turn everything off and keep checking it is off
  }
}

void loop() {
  #ifdef debug
    Serial.print(" Sensors: ");
    Serial.print("W = ");
    Serial.print(water_temp);
    Serial.print(" A = ");
    Serial.print(auger_temp);
    Serial.print(" F = ");
    Serial.print(flame_val);
    Serial.print("State = ");
    Serial.println(STATES_STRING[state]);
  #endif 
  //***************   safety  *********************
  safety();
  //*************  see if we need to turn on or off  **************
  //get flame_val average
  flame_value_median();
  //dz calls it: 1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == HIGH) {
    if ((state == STATE_HEATING) || (state == STATE_START_UP)) {
      this_state = STATE_IDLE;
      state = STATE_COOL_DOWN;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif  
    }
  }
  //button press?
//  if (digitalRead(BUTTON_1) == HIGH) {
//    if (debounce_start == 0) {
//      debounce_start = millis();
//    }
//    if ((long)(millis() - debounce_start) > BUTTON_ON_THRESHOLD) {
//      if (state != STATE_OFF) {
//        state = STATE_IDLE;
//        #ifdef mqtt
//          //
//          publish(STATE_TOPIC, STATES_STRING[state]);
//        #endif        
//      }else {
//        state = STATE_OFF;
//        #ifdef mqtt
//          //
//          publish(STATE_TOPIC, STATES_STRING[state]);
//        #endif        
//      }
//      debounce_start = 0;
//    }else if (digitalRead(BUTTON_1) == LOW) { //button not held long enough so don't turn switch state and reset counter
//      debounce_start = 0;
//    }
//  }
  //serial instruction
  if (stringComplete) {
    if (inputString.startsWith("Turn Off Boiler")) {
      inputString = "";
      reason = "";
      stringComplete = false;
      this_state = STATE_OFF;
      state = STATE_COOL_DOWN;
      #ifdef mqtt
        //
        publish(STATE_TOPIC, STATES_STRING[state]);
      #endif        
      //delay(10);
    }
    if (inputString.startsWith("Turn On Boiler")) {
      inputString = "";
      stringComplete = false;
      state = STATE_IDLE;
      reason = "";
      #ifdef mqtt
        //
        publish(ERROR_TOPIC, reason); //clear error register
      #endif    
      start_count = 0;  
      //delay(10);
    }else if (inputString.startsWith("Increase SetPoint")) {
      inputString = "";
      stringComplete = false;
      (int)temp_set_point++;
    }else if (inputString.startsWith("Decrease SetPoint")) {
      inputString = "";
      stringComplete = false;
      (int)temp_set_point--;
    }
  }
  
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
      proc_cool_down(this_state);
      break;
    case STATE_ERROR:
      proc_error();
      break;
     case STATE_OFF:
      proc_off();
      break;
  }
  if (stringComplete){
    inputString = "";
    stringComplete = false;
  }
  #ifdef mqtt
    if ((state == STATE_START_UP) or (state == STATE_HEATING) or (state == STATE_COOL_DOWN)) { //publish messages
      int mosqPayload[] = {water_temp, auger_temp, flame_val, state, fan_power, feed_percent, feed_pause_percent, temp_set_point};
      const String mosqTop[] = {WATER_TEMP_TOPIC, AUGER_TEMP_TOPIC, FLAME_TOPIC, STATE_TOPIC, PID_FAN, PID_FEED, PID_PAUSE, TEMP_SET_POINT_TOP};
      //publish everything in a round robin fashion
      if(millis() - previousMillis > PUB_INTERVAL) { //publish info
        previousMillis = millis();  
        if (index < 7 ) {
          index++; 
        }else {
          index = 0;
        }
        if (index==3){
         publish(mosqTop[index], STATES_STRING[state]);
        }else{
          publish(mosqTop[index], mosqPayload[index]);
        }
      }
    }else { //publish temp, pause for PUB_INTERVAL_IDLE, publish state, pause for PUB_INTERVAL_IDLE, publish temp etc
      if (index > 1) {//set it to value for publishing if was in higher value from prev on state
        index = 0;
      }
      if((long)(millis() - previousMillis) > PUB_INTERVAL_IDLE) { //publish info
        previousMillis = millis();
        if (index < 1) { 
          publish(WATER_TEMP_TOPIC, water_temp);//water temp in first member of arrays
          index++;
        }
        if (index == 1) {
          publish(STATE_TOPIC, STATES_STRING[state]);
          if (state == STATE_ERROR) {
            publish(ERROR_TOPIC, reason);
          }
          index = 0;
        }
      }
        
    }

  #endif
    #ifdef ac_counter
      if (crosses > 60) {
        crosses = 0;
        counts++;
        Serial.print("Number of counts = ");
        Serial.println(counts);
      }
    #endif
  //delay(200); //can't read form serial with delay
}
