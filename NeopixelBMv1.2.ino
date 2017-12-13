/* 
Neopixels on WeMos using IBM Bluemix By Andy Stanford-Clark - with embellishments by Lucy Rogers and a simple addition of Wifi connection Management by Tim Minter 
Edited for the Eaglelabs sessions by Jon Paterson 

 * Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at apache.org/licenses/LICEN… Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

// ACCESS_POINT_SETUP
#define ACCESS_POINT_NAME "EnchantedConnection"
#define ACCESS_POINT_PASSWORD "passwordHere"
// LIB
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

/////////////////////////////////////////////////////////////////////////////////////////////

// update this with the Broker address from IBM Watson IoT Platform

#define BROKER "Changeme.messaging.internetofthings.ibmcloud.com"

// update this with the Client ID in the format d:{org_id}:{device_type}:{device_id}

#define CLIENTID "d:Changeme:WEMOS:Coathook"

// update this with the authentcation token

#define PASSWORD "Authentcation token goes here"

/////////////////////////////////////////////////////////////////////////////////////////////


#define COMMAND_TOPIC "iot-2/cmd/command/fmt/text"

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(8, 4);
static uint32_t wifi_colour = pixel.Color(128, 0, 128);
static uint32_t mqtt_colour = pixel.Color(0, 128, 128);
static uint32_t current_colour = 0x000000;
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
  set_pixels(wifi_colour);   
  Serial.println();
  Serial.print("Connecting to wifi or setting up access point");
/////////////////////////////////////////////////////////////////////////////////////////////
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("Coathook#");
  
/////////////////////////////////////////////////////////////////////////////////////////////
 
  //reset settings - This is for engineers on the day to be able to remove the settings stored if issues.
  // un// the code below flash the device and then // and flash again.
  
  //wifiManager.resetSettings();
  
  wait_for_wifi();
  wifiManager.setTimeout(60);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char content[10];
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
  
  set_pixels(0);
 
  client.subscribe(COMMAND_TOPIC);
}


void set_colour(uint32_t colour) {
  
  set_pixels(colour);
  current_colour = colour;
}


void set_pixels(uint32_t colour) {
  
  for (int i = 0; i < pixel.numPixels(); i++) {
    pixel.setPixelColor(i, colour);
  }
  pixel.show();
  current_LED = colour;
}


void toggle_pixel() {

  if (current_LED == 0) 
  {
    set_pixels(current_colour);
  } 
  else
  {
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
