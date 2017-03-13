unsigned long debounce_start = 0; //button press debounce
unsigned long element_start = 0;
unsigned long fan_start = 0;
unsigned long auger_start = 0;
unsigned long reset_start_count_timer;
unsigned long state_trans_start = 0;
unsigned long state_trans_stop = 0;
unsigned long stop_start = 0; //for  fan short cycling
unsigned long error_timer = 0; //for dropping inot error in start loop
unsigned long start_feed_time = 0;//pellets
unsigned long start_feed_pause = 0;//pellets
unsigned long start_pump_time = 0;//pump short cycling
long timers[] = {debounce_start, element_start, fan_start, auger_start, 
                   reset_start_count_timer, state_trans_start, state_trans_stop, stop_start,
                   error_timer, start_feed_time, start_feed_pause, start_pump_time};


 char * users[]={
 "tom",
 "dick",
 "harry"
};

int numtimers (sizeof(timers)) //array size  



 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Number of members in array  = ");
  Serial.println(numtimers);
  
}
