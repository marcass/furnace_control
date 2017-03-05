

/*******************************
 BOILER CONTROL
 *******************************/
//#define debug
#define pid //use if PID controlling fan output
//#define no_PID //use if not pid controlling
#define mqtt
//#define ac_counter
//#define lcd //interferes with interrupts?
//#define fan


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

//Constants
const long ELEMENT_TIME = 360000; //6min in ms
const long START_FEED_TIME = 110000; //2min 10s in ms for pellet feed initially
const int LOW_TEMP = 50; //deg C -> low end of heating range
const int HIGH_TEMP = 75; //deg C -> high end of heating range
const int MID_TEMP = 68; //Send back to heating if over heats form here
const int TOO_HOT_TEMP = 85; //cool down NOW
const int AUGER_OVER_TEMP = 55; //deg C - don't want a hopper fire
const long START_FAN_TIME = 45000; //45s in ms for time to blow to see if flame present
const long END_FAN_TIME = 480000; //6min of blow to empty puck from burn box
const int FLAME_VAL_THRESHOLD = 120;//work out a value here that is reasonable
const int START_FLAME = 80;
const long PUMP_TIME = 30000; //30s in ms to avoid short cycling pump
const int BUTTON_ON_THRESHOLD = 1500;//1.5s in ms for turning from off to idle and vice versa
const long FEED_PAUSE = 40000; //60s and calculating a result so might need to be a float
const long FEED_TIME = 10000;
const long STATE_CHANGE_THRES = 5000;
const long STOP_THRESH = 5000; //for  fan short cycling
const long ERROR_THRES = 10000;
long RESET_THRESHOLD = 300000;


#ifdef mqtt
  long PUB_INTERVAL = 1000;
  long PUB_INTERVAL_STATE = 10000;
  long previousMillis = 0;
  const long PUB_INT = 60000; //publish values every minute
  const int STATE_PUB = 3;
  const int WATER_TEMP_PUB = 0;
  const int AUGER_TEMP_PUB = 1;
  const int FLAME_PUB = 2;
  const int ERROR_PUB = 4;
  int thisSens = 0;
  int sensorPub[] = {WATER_TEMP_PUB, AUGER_TEMP_PUB, FLAME_PUB, STATE_PUB};
  String STATE_TOPIC = "boiler/state";
  String WATER_TEMP_TOPIC = "boiler/temp/water";
  String AUGER_TEMP_TOPIC = "boiler/temp/auger";
  String FLAME_TOPIC = "boiler/flame";
  String ERROR_TOPIC = "boiler/messages";
#endif

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
unsigned long fallback_fan_start = 0;
unsigned long fan_time;
unsigned long element_time;
unsigned long auger_start = 0;
unsigned long auger_time;
int auger_temp;
int flame_val; // range from 0 to 1024 ( i think)
int state;
int start_count = 0;
unsigned long reset_start_count_timer;
bool reset = false;
String reason = "";
bool feeding = true;
bool cooling = false;
bool dump = false;
bool elem = false;
bool first_loop = true; //first off loop
bool runFan;
int proportion;
float divisor;
int crosses;
int counts;
unsigned long state_trans_start = 0;
unsigned long stop_start = 0; //for  fan short cycling
unsigned long error_timer = 0; //for dropping inot error in start loop

//I expect that code will alter these values in future
unsigned long start_feed_time = 0;
unsigned long start_feed_pause = 0;
unsigned long start_pump_time = 0;
unsigned long debounce_start = 0;
int on_wait; //variable for converting power to timer value
long feed_pause;
long feed_time;

#ifdef pid
  //PID setup
  //double Setpoint, Input, Output;
  double TEMP_SET_POINT = 68; //PID SETPOINT
  double power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
  double water_temp;
  double feed_pause_percent;
  double feed_percent;
  //Specify the links and initial tuning parameters
  //PID fanPID(&Input, &Output, &Setpoint,2,5,1, DIRECT);
  PID fanPID(&water_temp, &power, &TEMP_SET_POINT,2,5,1, DIRECT); //need more fan power to get hotter so DIRECT
  PID pausePID(&water_temp, &feed_pause_percent, &TEMP_SET_POINT,2,5,1, REVERSE); //need shorter feed time to get to hotter so REVERSE
  PID feedPID(&water_temp, &feed_percent, &TEMP_SET_POINT,2,5,1, DIRECT);
#endif

#ifdef no_PID
  int power; //variable for percentage power we want fan to run at works from 30 (min) to 80 (max)
  int water_temp; 
#endif

// SETUP SERIAL COMM for inputs
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete



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
  //Initialise inputs
  pinMode(DZ_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  #ifdef pid
    //initialize the PID variables we're linked to
    fanPID.SetOutputLimits(55, 80); //percentage of fan power
    fanPID.SetSampleTime(3000); //SAMPLES EVERY 3s
    pausePID.SetOutputLimits(60,100); //percentage of feed time check to see that burning all of load
    pausePID.SetSampleTime(3000);
    feedPID.SetOutputLimits(40,100); //percentage of feed time check to see that burning all of load
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
  attachInterrupt(0,zeroCrossingInterrupt, RISING);    
    //IRQ0 is pin 2. Call zeroCrossingInterrupt 
    //on rising signal
  
  // initialize serial communication:
  Serial.begin(9600);//slower speed for long cable
  inputString.reserve(200); // reserve mem for received message on serial port 
  int state = 0;
  #ifdef mqtt
    //initialise state as idle on statup
    Serial.print("MQTT:");
    Serial.print(STATE_TOPIC);
    Serial.print("/");
    Serial.println(state);
  #endif
}

//MQTT stuff
#ifdef mqtt
  void publish(int i) {
    if (i == WATER_TEMP_PUB) {
      Serial.print("MQTT:");
      Serial.print(WATER_TEMP_TOPIC);
      Serial.print("/");
      Serial.println(water_temp); 
    }else if (i== AUGER_TEMP_PUB) {
      Serial.print("MQTT:");
      Serial.print(AUGER_TEMP_TOPIC);
      Serial.print("/");
      Serial.println(auger_temp); 
    }else if (i == FLAME_PUB) {
      Serial.print("MQTT:");
      Serial.print(FLAME_TOPIC);
      Serial.print("/");
      Serial.println(flame_val);
    }else if (i == STATE_PUB) {
      Serial.print("MQTT:");
      Serial.print(STATE_TOPIC);
      Serial.print("/");
      if (state == STATE_IDLE) {
        Serial.println("Idle");
      }else if (state == STATE_START_UP) {
        Serial.println("Starting");
      }else if (state == STATE_HEATING) {
        Serial.println("Heating"); 
      }else if (state == STATE_COOL_DOWN) {
        Serial.println("Cool down");  
      }else if (state == STATE_ERROR) {
        Serial.println("Error");
      }else if (state == STATE_OFF) {
        Serial.println("Off");
      }
    }else if ( i == ERROR_PUB) {
      Serial.print("MQTT:");
      Serial.print(ERROR_TOPIC);
      Serial.print("/");
      Serial.println(reason); 
    }  
  }
#endif


//Interrupt Service Routines

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


//setup serial receive
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

void run_fan(int x) {
  if (x == 100) { //no phase angle control needed if you want balls out fan speed
    digitalWrite(GATE,HIGH);
    #ifdef debug
      Serial.print("  Fan on 100%  ");
    #endif
  }else {
    #ifdef debug
      Serial.print(" FAN ON at ");
      Serial.print(x);
    #endif
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
//    #ifdef debug
//      Serial.print("on_wait = ");
//      Serial.print(on_wait);
//      Serial.print("  ");
//    #endif
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
  if (stop_start == 0) { //dont' short cycle fan
    stop_start = millis();
  }
  if (millis() - stop_start > STOP_THRESH) {
    digitalWrite(GATE,LOW);
    #ifdef debug
      Serial.print("  Fan off  ");
    #endif 
    //undo phase angle magic here 
    TCCR1B = 0x00;
    TIMSK1 = 0x00;    //disable comparator A and overflow interrupts
    TCCR1A = 0x00;    //timer control registers set for
    TCCR1B = 0x00;    //normal operation, timer disabled
    stop_start = 0;
  }
}

void proc_idle() {
  //1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == LOW) {
    state = STATE_START_UP;
    runFan = true;
    reason = "";
    #ifdef mqtt
      //
      publish(STATE_PUB);
      publish(ERROR_PUB); //clear error register
    #endif
  }else {
    //twiddle thumbs
  }
}

void going_yet() {
  /*startup sequence
   * 1. check to see if heaps of light -> heating
   * 2. check to see if a little bit of light -> fan and go to 1
   * 3. if no light -> pellet dump and element start then go to 1
   */
  //read light in firebox
  flame_val = analogRead(LIGHT);
  if (flame_val > FLAME_VAL_THRESHOLD) { //if plenty of light go to heating
    if (state_trans_start == 0) { //smooth state transitions
      state_trans_start = millis();
    }
    if (millis() - state_trans_start > STATE_CHANGE_THRES) {
      
      state = STATE_HEATING;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif    
      fan_start = 0;
      element_start = 0;
      auger_start = 0;
      reset = true; //watch for timer to reset start count
      reset_start_count_timer = millis();
      state_trans_start = 0;
    } //else keep coming back to check
  }else if (flame_val > START_FLAME) { //a little bit of light so lets gently blow and see if flame_val_threshold breached
    runFan = true; //run fan at 40%
    dump = false;
    if (fan_start == 0) {
      fan_start = millis();
    }
    if (millis() - fan_start > START_FAN_TIME) { //start again
      runFan = false;
      dump = true;
      start_count++;
      fan_start = 0;
    }else {
      state_trans_start = 0; //not enough flame for state trans so reset timer
    }
  }
}

void proc_start_up() {
  //kill if failed to start too many times
  if (start_count > 2) {
    if (error_timer == 0) { //wait and think about shit for a while
      error_timer =  millis();
    }
    if (millis() - error_timer > ERROR_THRES) {
      state = STATE_ERROR;
      reason = "failed to start";
      #ifdef mqtt
        //
        publish(ERROR_PUB);
        publish(STATE_PUB);
        reason = "";
        error_timer = 0;
      #endif
    }
    runFan = true; //meanwhile, fan shit    
  }
  //run fan if true
  #ifdef debug
    Serial.print("runFan = ");
    Serial.print(runFan);
//    Serial.print("  Dump =   ");
//    Serial.print(dump);
    Serial.print("  ");
  #endif
  if (runFan) {
    run_fan(39);
  }else {
    stop_fan(); //fucking fan keeps turning on
  }
  going_yet(); //perform check in each loop
  //start fan on count 1 of each start up to see if flames present
  if (start_count == 0) {
    runFan = true;
    if (fan_start == 0) {
      fan_start = millis();
    }
    if (millis() - fan_start > START_FAN_TIME) {
      runFan = false;
      start_count++;
      dump = true;
      fan_start = 0;
    }
  }
  if (start_count > 0) {
    if (dump) {
      if (auger_start == 0) {
        auger_start = millis();
        
      }
      if (millis() - auger_start > START_FEED_TIME) {
        //stop feeding pellets
        digitalWrite(AUGER, LOW);
        dump = false;
        elem = true;
        #ifdef debug
          Serial.print("  Auger off  ");
        #endif
      }else {
        digitalWrite(AUGER, HIGH); //dump pellets
        runFan = false;
        digitalWrite(ELEMENT, LOW);
        #ifdef debug
          Serial.print(" Auger on ");
        #endif  
      }
    }     
    if (elem) {
      if (element_start == 0) { //start element timer if not already started
        element_start = millis();
      }
      //test to see if element been on for too enough and stop it if it has
      if (millis() - element_start > ELEMENT_TIME) {
        digitalWrite(ELEMENT, LOW);
//        #ifdef debug
//          Serial.println("Element on long enough so turned off");
//        #endif        
        runFan = true;//fan puck for a while to see if picked up in going yet
        if (fallback_fan_start == 0) {
          fallback_fan_start = millis();
        } //loop back to see if going for a bit
        if (millis() - fallback_fan_start > START_FAN_TIME) { //no flame made - hopeless so start again
          runFan = false;
          fallback_fan_start = 0;
          //go back to start and dump another load
          auger_start = 0;
          //increment start count
          start_count++;
          elem = false;
          dump = true;
        }
      }else {
        digitalWrite(ELEMENT, HIGH); //start element
        runFan = false;
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
}

void fan_and_pellet_management() {
  //water temp measured in proc_heating()
  /****************************************************
   * FAN MANAGEMENT
   ************************************************/
  if (water_temp < LOW_TEMP) { //go hard on the fan
    run_fan(100);
  }else if (water_temp > HIGH_TEMP) {
    state = STATE_COOL_DOWN;
    #ifdef mqtt
      //
      publish(STATE_PUB);
    #endif    
  }else {
    #ifdef no_PID
      power = 75; //arbitrary value
    #endif
    run_fan((int)power);
  }

  /**************************************************
   * PELLETS MANAGMENT
   * pid managment of pause between feeding and feed time
   **************************************************/
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
    if (millis() - start_feed_time > feed_time) {
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
    if (millis() - start_feed_pause > feed_pause) {
      //stop pausing, start feeding
      
      start_feed_pause = 0;
      feeding = true;
    }
  }
  #ifdef debug
    Serial.print(" Time left to feed = ");
    Serial.print(feed_time - (millis()-start_feed_time));
    Serial.print("  Time left in pause = ");
    Serial.println(feed_pause - (millis()-start_feed_pause));
//      Serial.print("  Feed pause percent = ");
//      Serial.print(feed_pause_percent);
  #endif
    
}

void pump(bool on) { //prevents short cycling of pump
  if (on) {
    digitalWrite(PUMP, HIGH);
    if (start_pump_time == 0) {    //avoid short cycling so pump for at least 30s
      start_pump_time = millis();
    }
  }
  if (!on) {
    if (millis() - start_pump_time > PUMP_TIME) {
      digitalWrite(PUMP, LOW);
      #ifdef debug
        Serial.print("  Pump off  ");
      #endif      
      start_pump_time = 0;
    }
  }
}

void proc_heating() {
  //do i want to have an intial hot run ot burn pellet load from start?
  #ifdef pid
    //set fan power and pellets pausse variable via PID lib here
    fanPID.Compute();
    pausePID.Compute();
    feedPID.Compute();
    #ifdef debug
      Serial.print("Fan power = ");
      Serial.print(power);
      Serial.print("%");
      Serial.print("  Feed time = ");
      Serial.print(feed_percent);
      Serial.print("%");
      Serial.print("  Pellets pause = ");
      Serial.print(feed_pause_percent);
      Serial.print("%  ");
    #endif
  #endif
  //test to see if dz aborts
  if (digitalRead(DZ_PIN) == HIGH) {
    state = STATE_COOL_DOWN;
    #ifdef mqtt
      //
      publish(STATE_PUB);
    #endif    
  }
  //dump pellets into flame box with timed auger runs
  //run the fan. 
  flame_val = analogRead(LIGHT);
  //If not enough flame start again
  if (flame_val < FLAME_VAL_THRESHOLD) {
    if (state_trans_start == 0) { //smooth state transitions
      state_trans_start = millis();
    }
    if (millis() - state_trans_start > STATE_CHANGE_THRES) {
      state = STATE_START_UP;
      runFan = true;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif    
      state_trans_start = 0;
    }else {//else keep coming back to check
      state_trans_start = 0; //enough flame for to remain in state so reset timer
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
    //kill fan if too hot
    if (water_temp > HIGH_TEMP) {
      state = STATE_COOL_DOWN;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif
    }
  }
  if (water_temp < LOW_TEMP) {
    pump(false);
  }
}

void cool_to_stop(int target_state) {
  //make sure stuff stays off
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  if (water_temp > TEMP_SET_POINT) {
    stop_fan();
  }else {
    run_fan(100);
  }
  #ifdef debug
    Serial.print("  Cooling down as all off  ");
  #endif 
  //Keep pumping heat into house until boiler cool
  if (water_temp < LOW_TEMP){
    pump(false);
    #ifdef debug
      Serial.print(" Pump off ");
    #endif
    if (fan_start == 0) {
      fan_start = millis();
    }
    if (flame_val > START_FLAME) {
      fan_start = 0; //still have light so reset final blow
    }
    if (millis() - fan_start > END_FAN_TIME) { 
      stop_fan(); //puck blown to peices, clean grate for next light
      if (state_trans_start == 0) { //smooth state transitions
        state_trans_start = millis();
      }
      if (millis() - state_trans_start > STATE_CHANGE_THRES) {
        if (target_state == STATE_OFF) {
        first_loop = true;
        start_count = 0;
        }
        state = target_state;
        #ifdef mqtt
          //
          publish(STATE_PUB);
        #endif   
        state_trans_start = 0;
      } //else keep coming back to check 
    } 
  }else {
    //start pump to dump heat
    pump(true);
    
    #ifdef debug
      Serial.print("  Pump on  ");
    #endif  
  }
}


void proc_cool_down() {
  /*order goes like this:
   * 1. Too hot? -> pump to get below mid range and check dz pin
   * 2. DZ pin LOW (heating) -> STATE_HEATING
   * 3. DZ pin HIGH (off) -> keep pumping/blowing until firebox empty and cooled boiler
   */
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  flame_val = analogRead(LIGHT);
  if (water_temp > TOO_HOT_TEMP) {
    cool_to_stop(STATE_ERROR);
    reason = "Too hot";
    #ifdef mqtt
      //
      publish(ERROR_PUB);
      reason = "";
    #endif 
  }
  if (water_temp > HIGH_TEMP) {
    stop_fan();
    //pump some water to cool it
    pump(true);
    #ifdef debug
      Serial.println("Fan, auger and element off, pump on TOO HOT");
    #endif    
  }else if (water_temp > MID_TEMP) {
    pump(true);
    run_fan(50); //gentle blow to keep things ticking over
  }
  //not too hot but now but could be cooler
  if (water_temp < MID_TEMP) {
    if (digitalRead(DZ_PIN) == LOW) {
      //get back to heating
      state = STATE_HEATING;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif      
    }
    if (digitalRead(DZ_PIN) == HIGH) {
      //no heat needed so empty fire box
      cool_to_stop(STATE_IDLE);
      start_count = 0;
    }
  }  
}

void proc_error() {
  #ifdef debug
    Serial.print("Everything off except pump maybe,  ");
    Serial.print(reason);
  #endif
  //kill heating thigns
  stop_fan();
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  //test if boiler too hot, if it is pump some water to cool it
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > MID_TEMP) {
    //start pump
    pump(true);
  }else {
    pump(false);
  }
  //turn off if serial comms received
  if (stringComplete) {
    if (inputString.startsWith("Turn Off Boiler")) {
      inputString = "";
      stringComplete = false;
      state = STATE_OFF;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif      
      //delay(10);
    }else {
      reason = inputString;
      #ifdef mqtt
        publish(ERROR_PUB); //dump message so we can see how it was malformed
      #endif
      reason = "";
      inputString = "";
      stringComplete = false;  
    }
  }
}

void proc_off() {
  //if too hot pump unitl not:
  if (water_temp > LOW_TEMP) {
    cool_to_stop(STATE_OFF);    
  }else if (first_loop) { //turn everthing off once
    start_count = 0;
    start_feed_time = 0;
    start_feed_pause = 0;
    start_pump_time = 0;
    debounce_start = 0;
    reason = "";
    stop_fan();
    digitalWrite(AUGER, LOW);
    digitalWrite(ELEMENT, LOW);
    pump(false);
    first_loop = false;
  }
  //turn on if serial comms received
  if (stringComplete) {
    if (inputString.startsWith("Turn On Boiler")) {
      inputString = "";
      stringComplete = false;
      state = STATE_IDLE;
      reason = "";
      #ifdef mqtt
        //
        publish(STATE_PUB);
        publish(ERROR_PUB); //clear error register
      #endif    
      start_count = 0;  
      //delay(10);
    }else {
      reason = inputString;
      #ifdef mqtt
        publish(ERROR_PUB); //dump message so we can see how it was malformed
      #endif
      reason = "";
      inputString = "";
      stringComplete = false;  
    }
  }
}

void safety() {
  water_temp = int(Thermistor(analogRead(WATER_TEMP)));
  if (water_temp > TOO_HOT_TEMP) {
    state = STATE_COOL_DOWN;
    #ifdef mqtt
      //
      publish(STATE_PUB);
    #endif    
  }
  //if auger too hot go into error
  auger_temp = int(Thermistor(analogRead(AUGER_TEMP)));
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
    #ifdef mqtt
      //
      publish(ERROR_PUB);
      publish(STATE_PUB);
      reason = "";
    #endif    
    #ifdef debug
      Serial.print("Auger temp is: ");
      Serial.print(auger_temp);
    #endif
  }
}


void loop() {
  safety();
  //see if we need to turn on or off
  //dz calls it: 1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == HIGH) {
    if ((state == STATE_HEATING) || (state == STATE_START_UP)) {
      state = STATE_COOL_DOWN;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif  
    }else {
      //do nothing i guess
    }
  }
  //button press?
  if (digitalRead(BUTTON_1) == HIGH) {
    if (debounce_start == 0) {
      debounce_start = millis();
    }
    if (millis() - debounce_start > BUTTON_ON_THRESHOLD) {
      if (state != STATE_OFF) {
        state = STATE_IDLE;
        #ifdef mqtt
          //
          publish(STATE_PUB);
        #endif        
      }else {
        state = STATE_OFF;
        first_loop = true;
        #ifdef mqtt
          //
          publish(STATE_PUB);
        #endif        
      }
      debounce_start = 0;
    }else if (digitalRead(BUTTON_1) == LOW) { //button not held long enough so don't turn switch state and reset counter
      debounce_start = 0;
    }
  }
  
  
  
  //serial instruction
  if (stringComplete) {
    if (inputString.startsWith("Turn Off Boiler")) {
      inputString = "";
      reason = "";
      stringComplete = false;
      state = STATE_OFF;
      first_loop = true;
      #ifdef mqtt
        //
        publish(STATE_PUB);
      #endif        
      //delay(10);
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
    Serial.println(state);
  #endif 
  if (stringComplete){
    inputString = "";
    stringComplete = false;
  }
  #ifdef mqtt
    if ((state == STATE_START_UP) or (state == STATE_HEATING) or (state == STATE_COOL_DOWN)) { //publish messages
      unsigned long currentMillis = millis();
      if(currentMillis - previousMillis > PUB_INTERVAL) {
        previousMillis = currentMillis;  
        if (thisSens < 4 ) {//0-2 are sensors, 3 is state
          thisSens++; 
        }else {
          thisSens = 0;
        }
        publish(sensorPub[thisSens]);
      }
    }
    if (state != STATE_OFF) {
      unsigned long currentMillis1 = millis();
      if(currentMillis1 - previousMillis > PUB_INTERVAL_STATE) {
        previousMillis = currentMillis1;  
        publish(STATE_PUB);
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
  if (reset) {
    if (millis() -  reset_start_count_timer > RESET_THRESHOLD) {
      start_count = 0;
      error_timer = 0;
      reset_start_count_timer = 0;
      reset = false;
    }
  }
}
