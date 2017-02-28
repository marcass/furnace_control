

/*******************************
 BOILER CONTROL
 *******************************/
#define debug
#define PID //use if PID controlling fan output
#define no_PID //use if not pid controlling

//Libraries
#include <Wire.h>
//https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
#include <LiquidCrystal_I2C.h>
//PID library
#include <math.h>
#include <PID_v1.h>
//for ac phase angle stuff
#include <avr/io.h>
#include <avr/interrupt.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
//need to set contrast and backlight and check pin assignments

//Constants
const int ELEMENT_TIME = 120000; //2min in ms
const int START_FEED_TIME = 30000; //30s in ms for pellet feed initially
const int LOW_TEMP = 45; //deg C -> low end of heating range
const int HIGH_TEMP = 75; //deg C -> high end of heating range
const int MID_TEMP = 65; //Send back to heating if over heats form here
const int TOO_HOT_TEMP = 85; //cool down NOW
const int AUGER_OVER_TEMP = 55; //deg C - don't want a hopper fire
const int START_FAN_TIME = 20000; //20s in ms for time to blow to see if flame present
const int FLAME_VAL_THRESHOLD = 300;//work out a value here that is reasonable
const int NO_FLAME = 100; 
const int START_FLAME = 200;
const int PUMP_TIME = 30000; //30s in ms
const int BUTTON_ON_THRESHOLD = 1500;//1.5s in ms for turning from off to idle and vice versa

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

//Variables that change
unsigned long element_start = 0;
unsigned long fan_start = 0;
unsigned long fan_time;
unsigned long element_time;
unsigned long auger_start = 0;
unsigned long auger_time;
int water_temp;
int auger_temp;
int flame_val; // range from 0 to 1024 ( i think)
int state = 0;
int start_count = 0;
String reason;
//I expect that code will alter these values in future
int feed_time = 5000; //5s in ms
int feed_pause = 20000; //20s in ms
unsigned long start_feed_time = 0;
unsigned long start_feed_pause = 0;
unsigned long start_pump_time = 0;
unsigned long debounce_start = 0;
int power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
int on_wait; //variable for converting power to timer value
bool fan_running;


//Outputs
const int PUMP = 2; //circ pump relay
const int FAN = 9; //fan relay. pin9 is a PWM pin and allows for analogWrite
const int AUGER = 3; //pellet auger relay
const int ELEMENT = 4; //fire starting element relay

//Inputs
const int WATER_TEMP = 3; //analogue pin 3
const int AUGER_TEMP = 6; //analogue pin 6
const int LIGHT = 2; //analogue pin 2 for flame detection
const int DZ_PIN = 5; // pin that is pulled up when 1-wire relay closes
const int DZ_SUPPLY = 6; //Need a pin to supply 5v that is passed to DZ_PIN when 1-wire relay closes
const int DETECT = 2; //ac detection pin
const int GATE = 9; //pwm pin
const int PULSE = 4;   //trigger pulse width (counts) that triac requires to fire one specified needs ~25mus

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
  //intitialise output pins (inputs are all analogue read pins)
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, HIGH);
  pinMode(GATE, OUTPUT);      //TRIAC gate control
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
  //Initialise inputs
  pinMode(DZ_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  // initialize serial communication:
  Serial.begin(115200);

  //set up timer1 for ac detection
  //(see ATMEGA 328 data sheet pg 134 for more details)
  OCR1A = 100;      //initialize the comparator
  //TIMSK1 = 0x03;    //enable comparator A and overflow interrupts - done in fan_running
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled
  // set up zero crossing interrupt
  //attachInterrupt(0,zeroCrossingInterrupt, RISING);    //Do this in run_fan()
    //IRQ0 is pin 2. Call zeroCrossingInterrupt 
    //on rising signal  
}

//Interrupt Service Routines

void zeroCrossingInterrupt(){ //zero cross detect   
  TCCR1B=0x04; //start timer with divide by 256 input
  TCNT1 = 0;   //reset timer - count from zero
}

ISR(TIMER1_COMPA_vect){ //comparator match
  digitalWrite(GATE,HIGH);  //set TRIAC gate to high
  TCNT1 = 65536-PULSE;      //trigger pulse width
}

ISR(TIMER1_OVF_vect){ //timer1 overflow
  digitalWrite(GATE,LOW); //turn off TRIAC gate
  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

void run_fan(int x) {
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
    attachInterrupt(0,zeroCrossingInterrupt, RISING);  // inturrupt 0 on digital pin 2
    //set a value that is a proportion of 520 for power
    on_wait = (520 - ((float)x / 100 * 520));
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

void stop_fan() {
  //undo phase angle magic here
  detachInterrupt(0);
  fan_running = false;
  digitalWrite(GATE,LOW);
  TCCR1B = 0x00;
  TIMSK1 = 0x00;    //disable comparator A and overflow interrupts
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled
}

void proc_idle() {
  //1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == LOW) {
    //see if already burning
    flame_val = analogRead(LIGHT);
    if (flame_val > FLAME_VAL_THRESHOLD) {
      state = STATE_HEATING;
      //stop_fan();
      fan_start = 0;
    }else { //if not already burning see if we can fan some flames
      if (fan_start == 0) {
        fan_start = millis();
        run_fan(25);
      }
    }
    //if still no light in fanning flames time kill fan and go to start up
    if (millis() - fan_start > START_FAN_TIME) {
      stop_fan();
      fan_start = 0;
      state = STATE_START_UP;
    }
  }
}

void proc_start_up() {
  //safety first
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
  }
  //kill if dz aborts
  if (digitalRead(DZ_PIN) == HIGH) {
    state = STATE_COOL_DOWN;
  }
  //read light in firebox
  flame_val = analogRead(LIGHT);
  //if plenty of light go to 

  
  //kill if failed to start too many times
  if (start_count > 2) {
    state = STATE_ERROR;
    reason = "failed to start";
  }
  //dump pellets to light with element
  if (auger_start == 0) {
    digitalWrite(AUGER, HIGH);
    auger_start = millis();
  }
  if (millis() - auger_start > START_FEED_TIME) {
    //stop feeding pellets
    digitalWrite(AUGER, LOW);
    //start element
    if (element_start == 0) {
      digitalWrite(ELEMENT, HIGH);
      element_start = millis();
    }
    if ((flame_val > START_FLAME) && (flame_val < FLAME_VAL_THRESHOLD)) { //little bit of light but needs help
      //gentle fanning to get going
      run_fan(20); //run fan at 20%
    }
    //test to see if element been on for too enough and stop it if it has
    if (millis() - element_start > ELEMENT_TIME) {
      //fan the flames until light is seen or wait threshhold is over
      digitalWrite(ELEMENT, LOW);
      element_start = 0;
      if (fan_start == 0) {
        fan_start = millis();
        run_fan(20); //run fan at 20%
      }
      if (flame_val > FLAME_VAL_THRESHOLD) {
        //we have flame, so:
        state = STATE_HEATING;
        start_count = 0;
        fan_start = 0;
      }
      //if still no light in fanning flames time kill fan and go back to start of start up
      if (millis() - fan_start > START_FAN_TIME) {
        stop_fan();
        fan_start = 0;
        //go back to start and dump another load
        auger_start = 0;
        //increment start count
        start_count = start_count++;
      }
    }
  }     
}

void fan_and_pellet_management() {
  //water temp measured in proc_heating()
  /****************************************************
   * FAN MANAGEMENT
   ************************************************/
  if (water_temp < LOW_TEMP) { //go hard on the fan
    run_fan(100);
  }else if (water_temp > TOO_HOT_TEMP) {
    state = STATE_COOL_DOWN;
  }else {
    #ifdef PID
      //set fan power variable via PID lib here
      //fancy maths give power = ?;
      
    #endif
    #ifdef no_PID
      power = 80; //arbitrary value
    #endif
    run_fan(power);
  }

  /**************************************************
   * PELLETS MANAGMENT
   **************************************************/
  if (start_feed_time == 0) {
    digitalWrite(AUGER, HIGH);
    start_feed_time = millis();
  }
  //test to see if feed been on for long enough
  if (millis() - start_feed_time > feed_time) {
    //stop feeding and start pausing
    digitalWrite(AUGER, LOW);
    if (start_feed_pause == 0) {
      start_feed_pause = millis();
    }
    if (millis() - start_feed_pause > feed_pause) {
      //stop pausing, start feeding
      start_feed_time = 0;
      start_feed_pause = 0;
    }
  }
}

void proc_heating() {
  //test to see if dz aborts
  if (digitalRead(DZ_PIN) == HIGH) {
    state = STATE_COOL_DOWN;
  }
  //dump pellets into flame box with timed auger runs
  //run the fan. Ideally pwm control based on PID info of temp change but simple for now
  flame_val = analogRead(LIGHT);
  //If not enough flame start again
  if (flame_val < FLAME_VAL_THRESHOLD) {
    state = STATE_START_UP;
  }
  //if auger too hot go into error
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
    #ifdef debug
      Serial.print("Auger temp is: ");
      Serial.println(auger_temp);
    #endif
  }
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  fan_and_pellet_management();
  //pump water when it is in the bands
  if (water_temp > LOW_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
    //avoid short cycling so pump for at least 30s
    start_pump_time = millis();
    //kill fan if too hot
    if (water_temp > TOO_HOT_TEMP) {
      state = STATE_COOL_DOWN;
    }
  }
  if (water_temp < LOW_TEMP) {
    //check to see we aren't short cycling pump
    if (millis() - start_pump_time > PUMP_TIME) {
      digitalWrite(PUMP, LOW);
      start_pump_time = 0;
    }
  }
}


void proc_cool_down() {
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  flame_val = analogRead(LIGHT);
  //kill heating thigns if too hot and keep checking to see what state needed
  if (water_temp > TOO_HOT_TEMP) {
    stop_fan();
    digitalWrite(AUGER, LOW);
    digitalWrite(ELEMENT, LOW);
    //pump some water to cool it
    digitalWrite(PUMP, HIGH);
  }
  
  //not too hot but now but could be cooler
  if ((water_temp < HIGH_TEMP) && (water_temp > MID_TEMP)) {
    //start pump
    digitalWrite(PUMP, HIGH);
    //blow fan until fire is out to empty firebox of pellet load
    run_fan(100);
  }else {
    if (digitalRead(DZ_PIN) == LOW) {
      //get back to heating
      state = STATE_HEATING;
    }
    if (digitalRead(DZ_PIN) == HIGH) {
      //no heat needed so empty fire box
      //blow fan until fire is out to empty firebox of pellet load
      run_fan(100);
      if (flame_val < NO_FLAME) {
        stop_fan();
      }
      //Keep pumping heat into house until boiler cool
      if (water_temp < LOW_TEMP){
        digitalWrite(PUMP, LOW);
        state = STATE_IDLE;
      }
    }
  }
}

void proc_error() {
  #ifdef debug
    Serial.println(reason);
  #endif
  //kill heating thigns
  digitalWrite(FAN, LOW);
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  //test if boiler too hot, if it is pump some water to cool it
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > LOW_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
  }else {
    digitalWrite(PUMP, LOW);
  }
}

void proc_off() {
  //settle arduino down:
  stop_fan();
  //if too hot pump unitl not:
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > HIGH_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
  }else {
    digitalWrite(PUMP, LOW);
  }
  //Wait for on either via button or serial comms
  //otherwise do nothing
//  if (digitalRead(BUTTON_1) == HIGH) {
//    if (debounce_start == 0) {
//      debounce_start = millis();
//    }
//    if (millis() - debounce_start > BUTTON_ON_THRESHOLD) {
//      state = STATE_IDLE;
//      debounce_start = 0;
//    }else if (digitalRead(BUTTON_1) == LOW) { //button not held long enough so don't turn on and reset counter
//      debounce_start = 0;
//    }
//  }
}

void manage_outputs() {
  //this shoudl be redundant but i'm paranoid
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > TOO_HOT_TEMP) {
    state = STATE_COOL_DOWN;
  }
  //if auger too hot go into error
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
    #ifdef debug
      Serial.print("Auger temp is: ");
      Serial.println(auger_temp);
    #endif
  }
}


void loop() {
  manage_outputs();
  //see if we need to turn on or off
  //Wait for on either via button or serial comms
  if (digitalRead(BUTTON_1) == HIGH) {
    if (debounce_start == 0) {
      debounce_start = millis();
    }
    if (millis() - debounce_start > BUTTON_ON_THRESHOLD) {
      if (state != STATE_OFF) {
        state = STATE_IDLE;
      }else {
        state = STATE_OFF;
      }
      debounce_start = 0;
    }else if (digitalRead(BUTTON_1) == LOW) { //button not held long enough so don't turn switch state and reset counter
      debounce_start = 0;
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
      proc_cool_down();
      break;
    case STATE_ERROR:
      proc_error();
      break;
     case STATE_OFF:
      proc_off();
      break;
    }
  #ifdef debug
    Serial.print("State = ");
    Serial.print(state);
  #endif 
}
