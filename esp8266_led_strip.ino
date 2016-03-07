#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


// #WiFi Details
const char* WIFI_SSID     = "XXXX";
const char* WIFI_PASS     = "XXXX";

// Cloud MQTT  Details
const char* MQTT_BROKER   =  "m21.cloudmqtt.com";
const short MQTT_PORT     =  18225;
const char* MQTT_USER     =  "XXXX";
const char* MQTT_PASS     =  "XXXX";
const short MQTT_SSL_PORT = 28225;
const short MQTT_WS_PORT  = 38225;
const char* MQTT_LED_TOP  = "LedStrip1";

// LED Data
const short MAX_LED_BRI   = 1024;
const short MIN_VIS_THRES = 100;

WiFiClient espClient;
PubSubClient client(espClient);

// All light variables are in order R,G,B

// Light state variables
bool powered_on = false;
bool light_on[3];
bool prev_light_on[3];
int light_brightness[3];
int prev_light_brightness[3];

// ESP8266 Data
const int REDPIN = 12;
const int GREENPIN = 13;
const int BLUEPIN = 16;

void save_previous_state(){
  for (int i=0;i<3;i++){
    prev_light_brightness[0] = light_brightness[0];
  }
}

// MQTT Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on Topic[");
  Serial.print(topic);
  Serial.print("] with message :");
  char* message = new char[length+1];
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  if (strcmp(message, "fullpower") == 0){
    for (int i=0;i<3;i++){
      light_brightness[i] = MAX_LED_BRI;
      light_on[i] = true;
    }
    analogWrite(REDPIN, light_brightness[0]);
    analogWrite(GREENPIN, light_brightness[1]);
    analogWrite(BLUEPIN, light_brightness[2]);  
  }
  else if (strcmp(message, "halfpower") == 0){
    for (int i=0;i<3;i++){
      light_brightness[i] = MAX_LED_BRI / 2;
      light_on[i] = true;
    }
    analogWrite(REDPIN, light_brightness[0]);
    analogWrite(GREENPIN, light_brightness[1]);
    analogWrite(BLUEPIN, light_brightness[2]);  
  }
  else if (strcmp(message, "power") == 0){
    if(light_on[0] || light_on[1] || light_on[2] )
    {
      for(int i=0;i<3;i++){
        prev_light_on[i] = light_on[i];
        light_on[i] = false;
        prev_light_brightness[i] = light_brightness[i];
      }
      analogWrite(REDPIN, 0);
      analogWrite(GREENPIN, 0);
      analogWrite(BLUEPIN, 0);
    }
    else { // not powered_on
      for (int i=0;i<3;i++)
      {
        if (prev_light_on[i]){
          light_on[i] = true;
          light_brightness[i] = prev_light_brightness[i]; 
        }
      }
      if(prev_light_on[0]){
        analogWrite(REDPIN, light_brightness[0]);
      }
      if(prev_light_on[1]){
        analogWrite(GREENPIN, light_brightness[1]);
      }
      if(prev_light_on[2]){
        analogWrite(BLUEPIN, light_brightness[2]);
      }
      
      if (light_brightness[0] < MIN_VIS_THRES && light_brightness[1] < MIN_VIS_THRES && light_brightness[2] < MIN_VIS_THRES){
        // Turn red for 1 second to notify brightness of all set to zero
        analogWrite(REDPIN, MAX_LED_BRI);
        delay(1000);
        analogWrite(REDPIN, 0);
        for (int i=0;i<3;i++)
        {
          light_brightness[i] = MAX_LED_BRI / 2;
        }
      }
    }
  }    
  else if (strcmp(message, "red") == 0){
    if (light_on[0]){
      analogWrite(REDPIN, 0);
    }
    else{
      analogWrite(REDPIN, light_brightness[0]);  
    }
    light_on[0] = !light_on[0]; 
  }
  else if (strcmp(message, "green") == 0){
    if (light_on[1]){
      analogWrite(GREENPIN, 0);
    }
    else{
      analogWrite(GREENPIN, light_brightness[1]);  
    }
    light_on[1] = !light_on[1]; 
  }
  else if (strcmp(message, "blue") == 0){
    if (light_on[2]){
      analogWrite(BLUEPIN, 0);
    }
    else{
      analogWrite(BLUEPIN, light_brightness[2]);  
    }
    light_on[2] = !light_on[2]; 
  }
  else if (strcmp(message, "brighter") == 0){
    for (int i=0;i<3;i++){
      if (light_on[i]){
        light_brightness[i] += light_brightness[i] * 0.1;
        if (light_brightness[i] > MAX_LED_BRI){
          light_brightness[i] = MAX_LED_BRI;
        }
      }
    }
    if (light_on[0]){
      analogWrite(REDPIN, light_brightness[0]);   
    }
    if (light_on[1]){
      analogWrite(GREENPIN, light_brightness[1]);   
    }
    if (light_on[2]){
      analogWrite(BLUEPIN, light_brightness[2]);   
    }
  }
  else if (strcmp(message, "dim") == 0){
    for (int i=0;i<3;i++){
      if (light_on[i]){
        light_brightness[i] -= light_brightness[i] * 0.1;
        if (light_brightness[i] < MIN_VIS_THRES){
          light_brightness[i] = 0;
        }
      }      
    }
    if (light_on[0]){
      analogWrite(REDPIN, light_brightness[0]);   
      if(light_brightness[0] == 0){
        light_on[0] = false;
      }        
    }
    if (light_on[1]){
      analogWrite(GREENPIN, light_brightness[1]);   
      if(light_brightness[1] == 0){
        light_on[1] = false;
      }
    }
    if (light_on[2]){
      analogWrite(BLUEPIN, light_brightness[2]);   
      if(light_brightness[2] == 0){
        light_on[2] = false;
      }
    } 
  }  // End Dim
  Serial.println();
  delete[] message;
}

void reconnect() {
  //   Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(MQTT_LED_TOP);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  } 
}

void setup()
{
  // set pin modes
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

  // begin serial and connect to WiFi
  Serial.begin(9600);
  delay(100);

  digitalWrite(REDPIN, HIGH);
  digitalWrite(GREENPIN, HIGH);
  digitalWrite(BLUEPIN, HIGH);
    
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  digitalWrite(REDPIN, LOW);
  digitalWrite(GREENPIN, LOW);
  digitalWrite(BLUEPIN, LOW);
  delay(300);

  // Initalise variables
  for (int i=0;i<3;i++){
    light_on[i] = false;
    light_brightness[i] = MAX_LED_BRI;
    prev_light_brightness[i] = light_brightness[i];
  }
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}  


