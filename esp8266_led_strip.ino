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

// Light state variables
bool powered_on = false;
bool red_on = false;
bool green_on = false;
bool blue_on = false;

int red_brightness = MAX_LED_BRI;
int blue_brightness = MAX_LED_BRI;
int green_brightness = MAX_LED_BRI;

int prev_red_brightness = red_brightness;
int prev_blue_brightness = blue_brightness;
int prev_green_brightness = green_brightness;
bool prev_red_on = false;
bool prev_green_on = false;
bool prev_blue_on = false;





// ESP8266 Data
const int REDPIN = 12;
const int GREENPIN = 13;
const int BLUEPIN = 16;

void save_previous_state(){
  prev_red_brightness = red_brightness;
  prev_blue_brightness = blue_brightness;
  prev_green_brightness = green_brightness;
}


// MQTT Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on Topic[");
  Serial.print(topic);
  Serial.print("Msg :");
  char* message = new char[length+1];
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  if (strcmp(message, "fullpower") == 0){
      green_brightness = red_brightness = blue_brightness = MAX_LED_BRI;
      green_on = red_on = blue_on = true;
      analogWrite(REDPIN, red_brightness);
      analogWrite(GREENPIN, green_brightness);
      analogWrite(BLUEPIN, blue_brightness);  
  }
  else if (strcmp(message, "halfpower") == 0){
      green_brightness = red_brightness = blue_brightness = MAX_LED_BRI / 2;
      green_on = red_on = blue_on = true;// clean
      analogWrite(REDPIN, red_brightness);
      analogWrite(GREENPIN, green_brightness);
      analogWrite(BLUEPIN, blue_brightness);  
  }
  else if (strcmp(message, "power") == 0){
    if(green_on || red_on || blue_on )
    {
      prev_red_on = red_on;
      prev_blue_on = blue_on;
      prev_green_on = green_on;
      Serial.println("During power switch, a light was on. Switching all off now");
      green_on = red_on = blue_on = false;
      prev_red_brightness = red_brightness;
      prev_green_brightness = green_brightness;
      prev_blue_brightness = blue_brightness;
      analogWrite(REDPIN, 0);
      analogWrite(GREENPIN, 0);
      analogWrite(BLUEPIN, 0);
    }
    else { // not powered_on
      Serial.println("Ok, not powered on currently");
      if (prev_red_on){
        red_on = true;
        red_brightness = prev_red_brightness;
        analogWrite(REDPIN, red_brightness);
      }
      if (prev_blue_on){
        blue_on = true;
        blue_brightness = prev_blue_brightness;
        analogWrite(BLUEPIN, blue_brightness);
      }
      if (prev_green_on){
        green_on = true;
        green_brightness = prev_green_brightness;
        analogWrite(GREENPIN, green_brightness);
      }

      if (blue_brightness < MIN_VIS_THRES && green_brightness < MIN_VIS_THRES && red_brightness < MIN_VIS_THRES){
        // Turn red for 1 second to notify brightness of all set to zero
        analogWrite(REDPIN, MAX_LED_BRI);
        delay(1000);
        analogWrite(REDPIN, 0);       
      }
    }
  }    
  else if (strcmp(message, "red") == 0){
    if (red_on){
      analogWrite(REDPIN, 0);
    }
    else{
      analogWrite(REDPIN, red_brightness);  
    }
    red_on = !red_on; 
    Serial.print("Red on is : ");
    Serial.println(red_on);
  }
  else if (strcmp(message, "green") == 0){
    if (green_on){
      analogWrite(GREENPIN, 0);
    }
    else{
      analogWrite(GREENPIN, green_brightness);  
    }
    green_on = !green_on; 
    Serial.print("Green on is : ");
    Serial.println(green_on);
  }
  else if (strcmp(message, "blue") == 0){
    if (blue_on){
      analogWrite(BLUEPIN, 0);
    }
    else{
      analogWrite(BLUEPIN, blue_brightness);  
    }
    blue_on = !blue_on; 
    Serial.print("blue on is : ");
    Serial.println(blue_on);
  }
  else if (strcmp(message, "bright") == 0){
    if (red_on){
       if(red_brightness < MIN_VIS_THRES){
         red_brightness = MIN_VIS_THRES;
       }
       red_brightness += red_brightness * 0.1;
       if (red_brightness > MAX_LED_BRI){
         red_brightness = MAX_LED_BRI;
       }
       Serial.print("Red Brightness is now: ");
       Serial.print(red_brightness);
       analogWrite(REDPIN, red_brightness);   
    }
    if (blue_on){
       if(blue_brightness < MIN_VIS_THRES){
         blue_brightness = MIN_VIS_THRES;
       }
       blue_brightness += blue_brightness * 0.1;
       if (blue_brightness > MAX_LED_BRI){
         blue_brightness = MAX_LED_BRI;
       }
       Serial.print("Blue Brightness is now: ");
       Serial.print(blue_brightness);
       analogWrite(BLUEPIN, blue_brightness);   
    }
    if (green_on){
       if(green_brightness < MIN_VIS_THRES){
         green_brightness = MIN_VIS_THRES;
       }
       green_brightness += green_brightness * 0.1;
       if (green_brightness > MAX_LED_BRI){
         green_brightness = MAX_LED_BRI;
       }
       Serial.print("Green Brightness is now: ");
       Serial.print(green_brightness);
       analogWrite(GREENPIN, green_brightness);   
    }
  }
  else if (strcmp(message, "dim") == 0){
    if (red_on){
      red_brightness -= red_brightness * 0.1;
      if (red_brightness < MIN_VIS_THRES){
        red_brightness = 0;
        red_on = false;
      }
      Serial.print("Red Brightness is now: ");
      Serial.print(red_brightness);
      analogWrite(REDPIN, red_brightness);   
    }
    if (blue_on){
      blue_brightness -= blue_brightness * 0.1;
      if(blue_brightness < MIN_VIS_THRES){
        blue_brightness = 0;
        blue_on = false;
      }
      Serial.print("Blue Brightness is now: ");
      Serial.print(blue_brightness);
      analogWrite(BLUEPIN, blue_brightness);   
    }
    if (green_on){
      green_brightness -= green_brightness * 0.1;
      if(green_brightness < MIN_VIS_THRES){
        green_brightness = 0;
        green_on = false;
      }
      Serial.print("Green Brightness is now: ");
      Serial.print(green_brightness);
      analogWrite(GREENPIN, green_brightness);   
    } 
  }  // End Dim
  Serial.println();
  delete[] message;
}

void reconnect() {
  // Loop until we're reconnected
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
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}  



