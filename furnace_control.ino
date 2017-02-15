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
const int MID_TEMP = 65; //Send back to heating if over heats form here
const int WATER_OVER_TEMP = 90; //deg C = Oh shit lets shut down
const int AUGER_OVER_TEMP = 55; //deg C - don't want a hopper fire
const int START_FAN_TIME = 20000; //20s in ms for time to blow to see if flame present
const int FLAME_VAL_THRESHOLD;//work out a value here that is reasonable
const int PUMP_TIME = 30000; //30s in ms
//States
const int STATE_IDLE = 0;
const int STATE_START_UP = 1;
const int STATE_HEATING = 2;
const int STATE_COOL_DOWN = 3;
const int STATE_ERROR = 4;

//Variables that change
unsigned long element_start = 0;
unsigned long fan_start = 0;
unsigned long fan_time;
unsigned long element_time;
unsigned long auger_start = 0;
unsigned long auger_time;
int water_temp;
int auger_temp;
int flame_val;
int state = 0;
int start_count = 0;
char reason;
//I expect that code will alter these values in future
int feed_time = 5000; //5s in ms
int feed_pause = 20000; //20s in ms
unsigned long start_feed_time = 0;
unsigned long start_feed_pause = 0;
unsigned long start_pump_time = 0;


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

m

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
  digitalWrite(DZ_SUPPLY, LOW); //pull up?
  //Initialise inputs
  pinMode(DZ_PIN, INPUT_PULLUP);
  digitalWrite(DZ_PIN, HIGH); 
  // don't think analogue pins need to be initialised pinMode(WATER_TEMP, INPUT);
  
  // initialize serial communication:
  Serial.begin(115200);
}

void proc_idle() {
  //1-wire relay gets closed by DZ3
  if (digitalRead(DZ_PIN) == LOW) {
    //see if already burning
    flame_val = analogRead(LIGHT);
    if (flame_val > FLAME_VAL_THRESHOLD) {
      state = STATE_HEATING;
      digitalWrite(FAN, LOW);
      fan_start = 0;
    }else { //if not already burning see if we can fan some flames
      if (fan_start == 0) {
        fan_start = millis();
        digitalWrite(FAN, HIGH); //watch for future PWM effects here
      }
    }
    //if still no light in fanning flames time kill fan and go to start up
    if (millis() - fan_start > START_FAN_TIME) {
      digitalWrite(FAN, LOW);
      fan_start = 0;
      state = STATE_START_UP;
    }
  }
}

void proc_start_up() {
  //safety first
  if (analogRead(AUGER_TEMP) > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
  }
  //kill if dz aborts
  if (digitalRead(DZ_PIN) == HIGH) {
    state = STATE_COOL_DOWN;
  }
  flame_val = analogRead(LIGHT);
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
    //test to see if element been on for long enough
    if (millis() - element_start > ELEMENT_TIME) {
      //fan the flames until light is seen or wait threshhold is over
      digitalWrite(ELEMENT, LOW);
      if (fan_start == 0) {
        fan_start = millis();
        digitalWrite(FAN, HIGH); //watch for future PWM effects here
      }
      if (flame_val > FLAME_VAL_THRESHOLD) {
        //we have flame, so:
        state = STATE_HEATING;
        start_count = 0;
        fan_start = 0;
      }
      //if still no light in fanning flames time kill fan and go back to start of start up
      if (millis() - fan_start > START_FAN_TIME) {
        digitalWrite(FAN, LOW);
        fan_start = 0;
        //go back to start and dump another load
        auger_start = 0;
        //increment start count
        start_count = start_count++;
      }
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
  auger_temp = analogRead(AUGER_TEMP);
  if (auger_temp > AUGER_OVER_TEMP) {
    state = STATE_ERROR;
    reason = "Auger too hot";
    #ifdef debug
      Serial.print("Auger temp is: ");
      Serial.println(auger_temp);
    #endif
  }
  digitalWrite(FAN, HIGH);
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
  //pump water when it is in the bands
  water_temp = analogRead(WATER_TEMP);
  if (water_temp > LOW_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
    //avoid short cycling so pump for at least 30s
    start_pump_time = millis();
    //kill fan if too hot
    if (water_temp > HIGH_TEMP) {
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
  //kill heating thigns
  digitalWrite(FAN, LOW);
  digitalWrite(AUGER, LOW);
  digitalWrite(ELEMENT, LOW);
  //test if boiler too hot, if it is pump some water to cool it
  water_temp = analogRead(WATER_TEMP);
  if (water_temp > MID_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
  }else {
    if (digitalRead(DZ_PIN) == LOW) {
      //get back to heating
      state = STATE_HEATING;
    }
    if (digitalRead(DZ_PIN) == HIGH) {
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
  water_temp = analogRead(WATER_TEMP);
  if (water_temp > LOW_TEMP) {
    //start pump
    digitalWrite(PUMP, HIGH);
  }else {
    digitalWrite(PUMP, LOW);
  }
}

void manage_outputs() {
  //this shoudl be redundant but i'm paranoid
  water_temp = analogRead(WATER_TEMP);
  if (water_temp > WATER_OVER_TEMP) {
    state = STATE_COOL_DOWN;
  }
  //if auger too hot go into error
  auger_temp = analogRead(AUGER_TEMP);
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
