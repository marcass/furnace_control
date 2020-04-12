#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const char willTopic[] = "furnace/will";
const char willMessage[] = "offline";
const char mqttServer[] = BROKER;
const int mqttPort = 1883;
const char DOOR_PUB[] = D_PUB;
const char DOOR_SUB[] = D_SUB;

// SETUP SERIAL COMM for inputs *********************************************************
//HardwareSerial Serial2(2);
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
#define RX2 16
#define TX2 17


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
      client.subscribe(D_SUB);
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
//  String s = String((char*)payload);
}

void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
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
  
  //boolean publish(const String &topic, const String &payload, bool retained, int qos);
//  if (client.publish(DOOR_PUB, char_array, false)){
  if (client.publish(topic, payload, false)){
    #ifdef debug
      Serial.println("Send successful");
    #endif
  }else{
    #ifdef debug
      Serial.println("Send failed");
    #endif
  }
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
