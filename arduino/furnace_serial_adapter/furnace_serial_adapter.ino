/* Wiring:
- Using esp32 wroom with oled screen
esp32     Logic level converter     arduion nano
------|--------------------------|---------------
15    | Low side to high side    | TX1
13    | Low side to high side    | RX0
3.3V  | LV                       |
GND   | GND                      | GND
      | HV                       | 5V

*/

#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char willTopic[] = "furnace/will";
const char willMessage[] = "offline";
const char mqttServer[] = BROKER;
const int mqttPort = 1883;
const char TOP_SUB[] = SUB;
int water_temp;
int auger_temp;
int state;
const char * state_array[] = {"Idle", "Starting", "Heating", "Cool Down", "Error", "Off"};

// SETUP SERIAL COMM for inputs *********************************************************
//HardwareSerial Serial2(2);
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
#define RX2 15
#define TX2 13

//*********************** Setup dislplay ************
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1  // GPIO0

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins) adn no reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//void serialEvent() {
//  while (Serial2.available()) {
//    Serial.println("data incoming");
//    // get the new byte:
//    char inChar = (char)Serial2.read();  // add it to the inputString:
//    inputString += inChar;
//    if (inChar == '\n') {
//      Serial.print("Incoming string is: ");
//      Serial.println(inputString);
//      stringComplete = true;
//    }
//  }
//}

WiFiClient wifi;
int status = WL_IDLE_STATUS;     // the Wifi radio's status
PubSubClient client(wifi);

//connection functions
//wifi
void connect_WIFI(){
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(MYSSID);
  WiFi.begin(MYSSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//mqtt
void reconnect_MQTT() {
  // Check wifi:
  if (WiFi.status() != WL_CONNECTED) {
    connect_WIFI();
  }
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a client ID
    String clientId = "furnace";
    // Attempt to connect
//    if (client.connect(clientId.c_str())) {
    if (client.connect(clientId.c_str(),willTopic, 0, true, willMessage)) {
      Serial.println("connected to mqtt broker");
      //Once connected, publish an empty message to offline topic...
      client.publish(willTopic, "online", true);
      // ... and resubscribe
      client.subscribe(TOP_SUB);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println("-------new message from broker-----");
  Serial.print("topic:");
  Serial.println(topic);
  Serial.print("data:");
  Serial.write(payload, length);
  Serial.println();
  payload[length] = '\0';
  String s = String((char*)payload);
  Serial2.println(s);
}

void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
  //Initialise display
  // Start I2C Communication SDA = 5 and SCL = 4 on Wemos Lolin32 ESP32 with built-in SSD1306 OLED
  Wire.begin(5, 4);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.println("Setting up boiler");
  display.display();
  delay(2000);
  connect_WIFI();
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
//  Serial2.begin(115200);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  reconnect_MQTT();
}

void send_stuff(String stuff){
  //  have to convert String to char array for mqtt lib
  int str_len = stuff.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  stuff.toCharArray(char_array, str_len);
  //split data into topic and payload
  char * topic;
  char * payload;
  char * topic_delimiter = "^";
  char * payload_delimiter = "\n";
  topic = strtok(char_array, topic_delimiter);
  payload = strtok(NULL, payload_delimiter);
  if (client.publish(topic, payload, false)){
    #ifdef debug
      Serial.println("Send successful");
    #endif
  }else{
    #ifdef debug
      Serial.println("Send failed");
    #endif
  }
  char * res;
  char * res1;
  char * res2;
  char * s1 = "state";
  char * s2 = "temp";
  char * s3 = "water";
  res = strstr(topic, s1);
  res1 = strstr(topic, s2);
  res2 = strstr(topic, s3);
  if (res) {
    alter_display(topic, payload, "state");
  }
  if (res1) {
    char * temp_type;
    char * s3 = "auger";
    temp_type = strstr(topic, s3);
    if (temp_type) {
      alter_display(topic, payload, "auger");
    }else if (res2) {
     alter_display(topic, payload, "water_temp");
    }
  }
}

void alter_display(char* topic, char* payload, String value) {
  if (value == "auger") {
    int temp = String(payload).toInt();
    if (temp != auger_temp) {
      auger_temp = temp;
      update_display();
    }
  }else if (value == "water_temp") {
      int temp = String(payload).toInt();
      if (temp != water_temp) {
        water_temp = temp;
        update_display();
      }
  }else if (value == "state") {
      int this_state = String(payload).toInt();
      if ( this_state != state) {
        state = this_state;
        update_display();
      }
  }
}

void update_display() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("State is ");
  display.println(state_array[state]);
  display.setCursor(0, 20);
  display.print("Water temp = ");
  display.println(water_temp);
  display.setCursor(0, 40);
  display.print("Auger temp = ");
  display.println(auger_temp);
  display.display();
}

void loop() {
  client.loop();
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();  // add it to the inputString:
    inputString += inChar;
    if (inChar == '\n') {
      Serial.print("Incoming string is: ");
      Serial.println(inputString);
      stringComplete = true;
    }
  }
  if (stringComplete){
    send_stuff(inputString);
    inputString = "";
    stringComplete = false;
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
