#include <ESP8266WiFi.h>
#include <PubSubClient.h> // http://pubsubclient.knolleary.net/
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson


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

// Light state variables
bool powered_on = false;
bool light_on[3];
bool prev_light_on[3];
short light_brightness[3];
short prev_light_brightness[3];
const short REDVAL = 0;
const short GREENVAL = 1;
const short BLUEVAL = 2;

// ESP8266 Data
const int REDPIN = 12;
const int GREENPIN = 13;
const int BLUEPIN = 16;

void save_previous_state(){
  for (int i=0;i<3;i++){
    prev_light_brightness[0] = light_brightness[0];
  }
}

void check_light_values()
{
  for (int i=0;i<3;i++){
    if(light_brightness[i] < MIN_VIS_THRES){
      light_brightness[i] = 0;
    }
  }
}

void set_lights(short light_values[3]){
  analogWrite(REDPIN, light_values[REDVAL]);
  analogWrite(GREENPIN, light_values[GREENVAL]);
  analogWrite(BLUEPIN, light_values[BLUEVAL]);
}

void light_alert(){
  // Turn red for 1 second to notify brightness of all set to zero
  analogWrite(REDPIN, MAX_LED_BRI);
  delay(1000);
  analogWrite(REDPIN, 0);
}

// MQTT Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on Topic[");
  Serial.print(topic);
  Serial.print("] with message :");
  char* message = new char[length+1];
  for (int i=0;i<length;i++) {
    // Serial.print((char)payload[i]);
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  
  // Create JSON Buffer  
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(message); 
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    light_alert();
    return;
  }
 
  const char* function_call = root["function"];
  Serial.println(function_call);

  /*    
   *     Possible functions:
   *     fullpower - turn on all lights at full power
   *     power - toggle on and off. On a power on, use previous settings
   *     set_lights - set value for each light with value passed
   */
  if (strcmp(function_call, "fullpower") == 0){
    for (int i=0;i<3;i++){
      light_brightness[i] = MAX_LED_BRI;
      light_on[i] = true;
    }
    set_lights(light_brightness);
  }
  else if (strcmp(function_call, "halfpower") == 0){
    for (int i=0;i<3;i++){
      light_brightness[i] = MAX_LED_BRI / 2;
      light_on[i] = true;
    }
    set_lights(light_brightness);
  }
  else if (strcmp(function_call, "power") == 0){
    if(light_on[REDVAL] || light_on[GREENVAL] || light_on[BLUEVAL] )
    {
      for(int i=0;i<3;i++){
        prev_light_on[i] = light_on[i];
        light_on[i] = false;
        prev_light_brightness[i] = light_brightness[i];
      }
      short light_val[3] = {0};
      set_lights(light_val);
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
        analogWrite(REDPIN, light_brightness[REDVAL]);
      }
      if(prev_light_on[1]){
        analogWrite(GREENPIN, light_brightness[GREENVAL]);
      }
      if(prev_light_on[2]){
        analogWrite(BLUEPIN, light_brightness[BLUEVAL]);
      }
      
      if (light_brightness[0] < MIN_VIS_THRES && light_brightness[1] < MIN_VIS_THRES && light_brightness[2] < MIN_VIS_THRES){
        light_alert();
        for (int i=0;i<3;i++)
        {
          light_brightness[i] = MAX_LED_BRI / 2;
        }
      }
    }
  }
  else if (strcmp(function_call, "set_lights") == 0){
    light_brightness[REDVAL] = root["red"].as<short>();
    light_brightness[GREENVAL] = root["green"].as<short>();
    light_brightness[BLUEVAL] = root["blue"].as<short>();
    set_lights(light_brightness);
  }
  else if (strcmp(function_call, "brighter") == 0){
    for (int i=0;i<3;i++){
      if (light_on[i]){
        light_brightness[i] += light_brightness[i] * 0.1;
        if (light_brightness[i] > MAX_LED_BRI){
          light_brightness[i] = MAX_LED_BRI;
        }
      }
    }
    if (light_on[REDVAL]){
      analogWrite(REDPIN, light_brightness[REDVAL]);   
    }
    if (light_on[GREENVAL]){
      analogWrite(GREENPIN, light_brightness[GREENVAL]);   
    }
    if (light_on[BLUEVAL]){
      analogWrite(BLUEPIN, light_brightness[BLUEVAL]);   
    }
  }
  else if (strcmp(function_call, "dim") == 0){
    for (int i=0;i<3;i++){
      if (light_on[i]){
        light_brightness[i] -= light_brightness[i] * 0.1;
        if (light_brightness[i] < MIN_VIS_THRES){
          light_brightness[i] = 0;
        }
      }      
    }
    if (light_on[REDVAL]){
      analogWrite(REDPIN, light_brightness[REDVAL]);   
      if(light_brightness[REDVAL] == 0){
        light_on[REDVAL] = false;
      }        
    }
    if (light_on[GREENVAL]){
      analogWrite(GREENPIN, light_brightness[GREENVAL]);   
      if(light_brightness[GREENVAL] == 0){
        light_on[GREENVAL] = false;
      }
    }
    if (light_on[BLUEVAL]){
      analogWrite(BLUEPIN, light_brightness[BLUEVAL]);   
      if(light_brightness[BLUEVAL] == 0){
        light_on[BLUEVAL] = false;
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

  short light_vals[3] = {512,512,512};
  set_lights(light_vals);
  //analogWrite(REDPIN, 512);
    
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

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  short light_val1[3] = {0};
  set_lights(light_val1);
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
