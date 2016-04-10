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
bool light_on[3];
short light_brightness[3];
const short REDVAL = 0;
const short GREENVAL = 1;
const short BLUEVAL = 2;

// ESP8266 Data
const int REDPIN = 12;
const int GREENPIN = 13;
const int BLUEPIN = 16;
const int PUSHBUTTON_PIN=15;

// working data
bool powerButtonOnState = false;

void set_lights(short light_values[3]){
  for(int i=0; i<3;i++){
    if (light_values[i] > 0){
      light_on[i] = true;
    }
    else{
      light_on[i] = false;
    }
  }
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

void publishUpdate(){

  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // Always work it back to the range 0-100 for the client
  root["red"]   = (int) (light_brightness[REDVAL]   / 10.24);
  root["green"] = (int) (light_brightness[GREENVAL] / 10.24);
  root["blue"]  = (int) (light_brightness[BLUEVAL]  / 10.24);

  char payload[100];
  root.printTo(payload, sizeof(payload));

  // Publish the msg and have broker retain for new controllers 
  bool retain = true;
  client.publish(MQTT_UPDATE_TOPIC, payload, retain);
}

// MQTT Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on Topic[");
  Serial.print(topic);
  Serial.print("] with message :");
  char* message = new char[length+1];
  for (int i=0;i<length;i++) {
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
   *     brighter - brighten lights by 10%
   *     dim - dim lights by 10%
   */
  
  if (strcmp(function_call, "setLights") == 0){
    light_brightness[REDVAL] = 10.24 * root["red"].as<short>();
    light_brightness[GREENVAL] = 10.24 * root["green"].as<short>();
    light_brightness[BLUEVAL] = 10.24 * root["blue"].as<short>();
    set_lights(light_brightness);
  }
  else if (strcmp(function_call, "brightness") == 0){

    float brightness = 10.24 * root["brightVal"].as<int>();

    // find brightest colour to base brightness value off
    short highestVal = light_brightness[0];
    for (int i=1;i<3;i++){
      if(light_brightness[i] > highestVal){
        highestVal = light_brightness[i];
      }
    }

    if (highestVal == 0) { // All set to off
      light_brightness[REDVAL] = light_brightness[GREENVAL] = light_brightness[BLUEVAL] = brightness;
    }
    else if (brightness > highestVal){ // Brighten Lights
      float brightnessFactor =  ((brightness - highestVal) / highestVal);
      light_brightness[REDVAL] =   light_brightness[REDVAL] + (light_brightness[REDVAL]   * brightnessFactor);
      light_brightness[GREENVAL] = light_brightness[GREENVAL] + (light_brightness[GREENVAL] * brightnessFactor);
      light_brightness[BLUEVAL] =  light_brightness[BLUEVAL] + (light_brightness[BLUEVAL]  * brightnessFactor);
    }
    else { // Dim Lights
      float brightnessFactor = ((highestVal - brightness) / highestVal);
      light_brightness[REDVAL] =   light_brightness[REDVAL] - (light_brightness[REDVAL]   * brightnessFactor);
      light_brightness[GREENVAL] = light_brightness[GREENVAL] - (light_brightness[GREENVAL] * brightnessFactor);
      light_brightness[BLUEVAL] =  light_brightness[BLUEVAL] - (light_brightness[BLUEVAL]  * brightnessFactor);
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
  else if (strcmp(function_call, "redBrightness") == 0){
    int brightness = root["redVal"].as<int>();
    
    light_brightness[REDVAL] = brightness * 10.24;
    
    if (light_brightness[REDVAL] > MIN_VIS_THRES)
    {
      light_on[REDVAL] = true;
    }
    
    if (light_on[REDVAL]){
      analogWrite(REDPIN, light_brightness[REDVAL]);   
    }
    else{
      Serial.println("Red Light is not switched on");
    }
  }
  else if (strcmp(function_call, "greenBrightness") == 0){
    int brightness = root["greenVal"].as<int>();
    
    light_brightness[GREENVAL] = brightness * 10.24;
    
    if (light_brightness[GREENVAL] > MIN_VIS_THRES)
    {
      light_on[GREENVAL] = true;
    }
    
    if (light_on[GREENVAL]){
      analogWrite(GREENPIN, light_brightness[GREENVAL]);   
    }
    else{
      Serial.println("Green Light is not switched on");
    }
  }
  else if (strcmp(function_call, "blueBrightness") == 0){
    int brightness = root["blueVal"].as<int>();
    
    light_brightness[BLUEVAL] = brightness * 10.24;
    
    if (light_brightness[BLUEVAL] > MIN_VIS_THRES)
    {
      light_on[BLUEVAL] = true;
    }
    
    if (light_on[BLUEVAL]){
      analogWrite(BLUEPIN, light_brightness[BLUEVAL]);   
    }
    else{
      Serial.println("Blue Light is not switched on");
    }
  }
  else if(strcmp(function_call, "diagnostic") == 0){
    Serial.println("Diagnostic Information");
    Serial.println("----------------------");
    Serial.print("Red on ");
    Serial.print(light_on[REDVAL]);
    Serial.print(" and value of: ");
    Serial.println(light_brightness[REDVAL]);
    Serial.print("Green on ");
    Serial.print(light_on[GREENVAL]);
    Serial.print(" and value of: ");
    Serial.println(light_brightness[GREENVAL]);
    Serial.print("Blue on ");
    Serial.print(light_on[BLUEVAL]);
    Serial.print(" and value of: ");
    Serial.println(light_brightness[BLUEVAL]);
    
  }
  else {
    Serial.println("Function not found - are you sure it is correct?"); 
  }
  publishUpdate();
  delete[] message;
}

void reconnect() {
  //   Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(MQTT_LED_TOPIC);
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
  pinMode(PUSHBUTTON_PIN, INPUT);
  
  // begin serial and connect to WiFi
  Serial.begin(9600);
  delay(100);

  short light_vals[3] = {900,0,0};
  set_lights(light_vals);

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
    light_brightness[i] = 0;
  }
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  int buttonState = digitalRead(PUSHBUTTON_PIN);
  if (buttonState == HIGH && !powerButtonOnState){
     short light_vals[3] = {900,900,900};
     set_lights(light_vals); 
     powerButtonOnState = true;
     delay(500);
  }
  else if (buttonState == HIGH && powerButtonOnState)
  {
    short light_vals[3] = {0};
    set_lights(light_vals);
    powerButtonOnState = false;
    delay(500);
  }  
}  
