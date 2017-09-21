/* 
 *  Neopixels on WeMos using IBM Bluemix

Neopixel test using a WeMoS D1 ESP-8266 board and Node-RED on IBM Bluemix
expects messages of the form: "#rrggbb" and sets the LEDs to that colour


By Andy Stanford-Clark - with embellishments by Lucy Rogers and a simple addition of Wifi connection Management by Tim Minter 
*/

// You will need to install the "WiFiManager" library by Tzapu via Library Manager.

// If the board is started, and a known wifi connection is available, it will connect automatically. 
// If no known connection is found the board will switch to access point mode and a wifi network will be created with the ACCESS_POINT_NAME set below. 
// Using anotherdevice you cna the connect to that wifi connection using the ACCESS_POINT_PASSWORD set below.
// Your browser will be opened to a configuration page where you can set up the wifi credentials for this board.


// ACCESS_POINT_SETUP
#define ACCESS_POINT_NAME "EnchantedConnection"
#define ACCESS_POINT_PASSWORD "passwordHere"

 /* 
 * Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * apache.org/licenses/LICEN…
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 

 use board "WeMos D1 R2 & mini"
 CPU 160MHz
 4M (3M SPIFFS)
 upload speed 115200
 
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// remember to change MQTT_KEEPALIVE to 60 in the header file {arduino installation}/libraries/PubSubClient/src/PubsSubClient.h

/////////////////////////////////////////////////////////////////////////////////////////////

// update this with the Broker address from IBM Watson IoT Platform
#define BROKER "v4sxgx.messaging.internetofthings.ibmcloud.com"
// update this with the Client ID in the format d:{org_id}:{device_type}:{device_id}
// eg d:et6ddf:Wemos:WemosDoorKnob
#define CLIENTID ""
// update this with the authentcation token
#define PASSWORD ""

/////////////////////////////////////////////////////////////////////////////////////////////


// subscribe to this for commands:
#define COMMAND_TOPIC "iot-2/cmd/command/fmt/text"

WiFiClient espClient;
PubSubClient client(espClient);

//Change this if using different number of neopixels or different pin
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, 4); // eight pixels, on pin 4
// pin 4 is "D2" on the WeMoS D1 mini


// flashes this colour when connecting to wifi:
static uint32_t wifi_colour = pixel.Color(128, 0, 128); // magenta
// flashes this colour when connecting to MQTT:
static uint32_t mqtt_colour = pixel.Color(0, 128, 128); // cyan

static uint32_t current_colour = 0x000000; // black
static uint32_t current_LED = current_colour;



void setup() {
  
  Serial.begin(9600);

  pixel.begin();
  pixel.show(); // Initialize all pixels to 'off'
  
  setup_wifi();
  client.setServer(BROKER, 1883);
  client.setCallback(callback);
  
}


void setup_wifi() {
  
  // connecting to wifi colour
  set_pixels(wifi_colour); 
  
  // Start by connecting to the WiFi network   
  Serial.println();
  Serial.print("Connecting to wifi or setting up access point");
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("EnchantedSetup","S3tMeUpNow");
  
  wait_for_wifi();
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}


void callback(char* topic, byte* payload, unsigned int length) {
  char content[10];
  
  // #rrggbb
  
  Serial.print("Message arrived: ");

  if (length != 7)
  {
    Serial.print("expected 7 bytes, got ");
    Serial.println(length);
    return;
  }

  // "else"...

  payload[7] = '\0';
  
  Serial.print("'");
  Serial.print((char *)payload);
  Serial.println("'");

  // "+1" to skip over the '#'
  strcpy(content, (char *)(payload + 1));

  // convert the hex number to decimal
  uint32_t value = strtol(content, 0, 16);

  set_colour(value);
}


void wait_for_wifi()
{
  
  Serial.println("waiting for Wifi");
  
  // connecting to wifi colour
  set_colour(wifi_colour);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    toggle_pixel();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}


void reconnect() {
  boolean first = true;

  // Loop until we're reconnected to the broker  
  while (!client.connected()) {

    if (WiFi.status() != WL_CONNECTED) {
      wait_for_wifi();
      first = true;
    }
    
    Serial.print("Attempting MQTT connection...");
    if (first) {
      // now we're on wifi, show connecting to MQTT colour
      set_colour(mqtt_colour);
      first = false;
    }
    
    // Attempt to connect
    if (client.connect(CLIENTID, "use-token-auth", PASSWORD)) {
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      
      toggle_pixel();
    }
  }
  
  set_pixels(0); // clear pixel when connected (black)
 
  // subscribe to the command topic
  client.subscribe(COMMAND_TOPIC);
}


void set_colour(uint32_t colour) {
  
  set_pixels(colour);
  // Update current_LED with what the user last requested,
  // so we can toggle it to black and back again.
  current_colour = colour;
}


void set_pixels(uint32_t colour) {
  
  for (int i = 0; i < pixel.numPixels(); i++) {
    pixel.setPixelColor(i, colour);
  }
  pixel.show();

  // Store current actual LED colour
  // (which may be black if toggling code turned it off.)
  current_LED = colour;
}


void toggle_pixel() {

  if (current_LED == 0) 
  {
    // if it's black, set it to the stored colour
    set_pixels(current_colour);
  } 
  else
  {
    // otherwise set it to black
    set_pixels(0);
  }
}


void loop() {
  
  if (!client.connected()) {
    reconnect();
  }

  // service the MQTT client
  client.loop();
}
