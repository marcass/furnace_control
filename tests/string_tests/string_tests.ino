unsigned long debounce_start = 0; //button press debounce
unsigned long element_start = 1;
unsigned long fan_start = 2;
unsigned long auger_start = 3;
unsigned long reset_start_count_timer = 200;
unsigned long state_trans_start = 5;
unsigned long state_trans_stop = 6;
unsigned long stop_start = 7; //for  fan short cycling
unsigned long error_timer = 8; //for dropping inot error in start loop
unsigned long start_feed_time = 9;//pellets
unsigned long start_feed_pause = 10;//pellets
unsigned long start_pump_time = 11;//pump short cycling
unsigned long timers[] = {debounce_start, element_start, fan_start, auger_start, 
                   reset_start_count_timer, state_trans_start, state_trans_stop, stop_start,
                   error_timer, start_feed_time, start_feed_pause, start_pump_time};
//
//debounce_start = 0; //button press debounce
//element_start = 1;
//fan_start = 2;
//auger_start = 3;
//reset_start_count_timer = 200;
//state_trans_start = 5;
//state_trans_stop = 6;
//stop_start = 7; //for  fan short cycling
//error_timer = 8; //for dropping inot error in start loop
//start_feed_time = 9;//pellets
//start_feed_pause = 10;//pellets
//start_pump_time = 11;//pump short cycling

String blah = "hello blah";

int numtimers (sizeof(timers)/sizeof(long)); //array size  


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  Serial.print("Reset start count timer = ");
  Serial.println(reset_start_count_timer);
  // put your main code here, to run repeatedly:
//  Serial.print("Number of members in array  = ");
//  Serial.print(numtimers);
  for (int i = 0; i < numtimers; i++) {
    Serial.print("timers = ");
    Serial.println(timers[i]);
    if (timers[i] != 0) {
      timers[i] = 0;
      Serial.print("reset timer = ");
      Serial.println(timers[i]);
    }
    delay(50);
  }
  delay(1000);
//  if (reset_start_count_timer != 0) {
//    reset_start_count_timer = 0;
//  }
//  Serial.print("Reset start count timer = ");
  Serial.println(String("Is blah there" + blah));
}
