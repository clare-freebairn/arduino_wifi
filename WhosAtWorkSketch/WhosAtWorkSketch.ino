//#include <WiFi.h>
#include <Dictionary.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

//Light things:
#include <Adafruit_NeoPixel.h>
#define LED_PIN 8
#define BUTTON_PIN 7
#define LED_COUNT 120

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//WiFi connectivity variables:
WiFiSSLClient client;
char ssid[] = SECRET_SSID;        // network SSID
char pass[] = SECRET_PASS;    //network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

//Pins:
int ONLINE_BUTTON_PIN = 7;
int AWAY_BUTTON_PIN = 6;
int BREAK_BUTTON_PIN = 5;
int OFFLINE_BUTTON_PIN = 4;
int ON_ONE_OR_ZERO = 0;



//Global variables:
int button;
bool online;
bool away;
bool on_break;
bool offline;
bool send_online;
bool send_away;
bool send_on_break;
bool send_offline;
String my_name = "/Clare";


int HTTP_PORT = 80;
char HOST_NAME[] = "who-s-at-work-a7zmhe11fk5c.runkit.sh";
String HTTP_METHOD = "GET"; // or POST
String SET_METHOD = "/set_status";
String STATUS_METHOD = "/status";
String LISTEN_METHOD = "/status_changed";

//JSON
#include <ArduinoJson.h>

void setup() {
  online = false;
  Serial.begin(9600);

  //Initialise LEDS:
  strip.begin(); //always needed
  strip.show(); // 0 data => off.
  strip.setBrightness(50); // ~20% (max = 255)

  //WIFI setup code will go here:

  connectWIFI();

  //Initialising the Buttons and LEDs.
  pinMode(ONLINE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Debugging:
  //  Serial.println(HOST_NAME, HTTP_PORT);


}

//This function is copied from the tutorial on Arduino.cc
//about how to connect Arduino MKR WiFi 1010 to a network.
void connectWIFI() {

  while (!Serial);

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect to network: ");
    Serial.println(ssid);

    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);

    Serial.println("Status: " + String(status));
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

}

//Also taken from Arduino.cc tutorial
void printData() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

}

void loop() {

  turnOnLight(online);
  Serial.print(online);

  checkButtons();
  getStatus();
  delay(200);
}

void checkButtons() {
  if (digitalRead(ONLINE_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    online = !online;
    send_online = true;
  }

  if (digitalRead(AWAY_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    away == !away;
    send_away = true;
  }

  if (digitalRead(BREAK_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    on_break == !on_break;
    send_on_break = true;
  }

  if (digitalRead(OFFLINE_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    offline == !offline;
    send_offline = true;
  }

}

void send_status_request() {
  String path = "/status";
  String HTTP_version = "1.0";
  Serial.println("Making HTTP Get request to /status");
  client.println("GET " + path + " HTTP/" + HTTP_version);
  client.println("Host: " + String(HOST_NAME));
  client.println("Connection: close");
  client.println();
}

DynamicJsonDocument process_status_response() {
  Serial.println("Awaiting response");
  Serial.println("Delaying to enable complete transmission");
  delay(1000);

  // Some code below from https://github.com/bblanchon/ArduinoJson/blob/6.x/examples/JsonHttpClient/JsonHttpClient.ino

  // Allocate the JSON document
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument status_dictionary(capacity);

  // Skip HTTP headers by looking for double new line
  char endOfHeaders[] = "\r\n\r\n";

  // Checking to see if we find new lines
  if (!client.find(endOfHeaders)) {

    // New lines not found!
    Serial.println("Invalid response, failed to find the double newline characters for body of response.");
    client.stop();
    return status_dictionary;
  }

  // Parse JSON object
  DeserializationError error = deserializeJson(status_dictionary, client);

  // Check if we had error parsing
  if (error) {
    Serial.println("deserializeJson() failed: ");
    Serial.println(error.f_str());
    client.stop();
    return status_dictionary;
  }
  else {
    Serial.println("Successfully parsed json.");

    // the server's disconnected, stop the client:
    client.stop();
    Serial.println();
    Serial.println("disconnected");

    // Return the new statuses!
    return status_dictionary;
  }
}


void getStatus() {
  //Unfinished
  if (send_online == true) {
    send_online = false;

    Serial.println("Attempting to retrieve status");
    if (client.connect(HOST_NAME, 443)) {
      Serial.println("Connection to server successful");

      // Make the HTTPS status request
      send_status_request();

      // Ensure we are connected to receive response
      if (client.connected()) {
        // Parse dictionary from the resulting stream
        DynamicJsonDocument status_dictionary = process_status_response();

        // Pass in received dictionary to status update
        handle_status_update(status_dictionary);
      } else {
        Serial.println("Not connected to server after sending status request. :-(");
      }
    }
  }
}

void handle_status_update(DynamicJsonDocument status_dictionary) {
  // Check if a name is found in the dictionary
  if (status_dictionary.containsKey("Clare")) {
    // If it is, then let's get the status
    String clare_status = status_dictionary["Clare"];

    // Print it out and alert lubby Clare that the JSON is ready for action!
    Serial.println("Ready to handle JSON, parsed successfully, Clare's status is: " + clare_status);
  }
  else {
    Serial.println("Couldn't find Clare in the status dictionary. Did something go wrong when parsing?");
  }
}


void turnOnLight(bool on) {
  //Turns on the lights that correspond to each status

  //If online is true, turn on
  if (on) {
    makeLights();
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  //If offline is true, turn on

  //If away is true, turn on.

  //If Break is true, turn on.

}


void makeLights() {

  int n = strip.numPixels();
  // fph = FirstPixelHue;
  for (long fph = 0; fph < 5 * 65536; fph += 1024)
  {
    for (int i = 0; i < n; i++)
    {
      int pixelHue = fph + (i * 65536L / n);
      strip.setPixelColor(i,
                          strip.gamma32(strip.ColorHSV(pixelHue))
                         );
    }
    strip.show();
    delay(10);
  }
}
