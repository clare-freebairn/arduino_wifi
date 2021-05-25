//Included Libraries and external Files:
#include <WiFiNINA.h>
#include <Chrono.h>
#include <Adafruit_NeoPixel.h>
#include "arduino_secrets.h"

//+++++++++++++++++ Variables and Classes +++++++++++++++++

//LIGHTS SETUP:
#define LED_PIN 8
#define BUTTON_PIN 7
#define LED_COUNT 120

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//SETTING UP PEOPLE CLASS 
// Total people
const int PEOPLE_COUNT = 8;

// List of names (change order to match LED index on strip)
String names[PEOPLE_COUNT] = {"Clare", "Molly", "Connor", "Andrew", "Peter", "Hamish", "Oliver", "Pia"};

// Possible statuses
enum Status {Online, Away, Offline, On_Break};

// Count of statuses
const int STATUS_COUNT = 4;

// Status strings
const String STATUS_STRINGS[STATUS_COUNT] = {"Online", "Away", "Offline", "On Break"};




// Holds all info for a person
class Person {
  public:
    // Index on LED strip
    int led;

    String name;

    // Current status (enum)
    Status status;
};

class Colour {
  public:

    // RGB values for colour
    int R; // 0-255
    int G;
    int B;

    // Create colour with initial values
    Colour(int red_amount, int green_amount, int blue_amount) {
      R = red_amount;
      G = green_amount;
      B = blue_amount;
    };

    // Default colour initializer
    Colour() {
      R = 0;
      G = 0;
      B = 0;
    }
};

// Status colours
Colour STATUS_COLOURS[STATUS_COUNT];

// All of the people! (Their Person classes)
Person people[PEOPLE_COUNT];


//WIFI CONNECTION THINGS:
WiFiSSLClient client;
char ssid[] = SECRET_SSID;        // network SSID
char pass[] = SECRET_PASS;    //network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

Chrono status_request_timer;

//Pins:
int ONLINE_BUTTON_PIN = 7;
int AWAY_BUTTON_PIN = 6;
int BREAK_BUTTON_PIN = 5;
int OFFLINE_BUTTON_PIN = 4;
int ON_ONE_OR_ZERO = 0;



//GLOBAL VARIABLES:
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

//+++++++++++++++++ Things that are run in Setup +++++++++++++++++

void setup() {

  //Initialise status booleans:
  online = false;
  away = false;
  on_break = false;
  offline = false;
  Serial.begin(9600);
  
  //Initialising the Buttons and LEDs.
  pinMode(ONLINE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(AWAY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BREAK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OFFLINE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  //Initialise LEDS:
  strip.begin(); //always needed
  strip.show(); // 0 data => off.
  strip.setBrightness(50); // ~20% (max = 255)
  setupPixelColours(); // setting up the Colour classes
  
  //Connect to local network:
  connectWIFI();

  // Get initial statuses from the server
  getStatus();
}

void setupPixelColours() {

  // Setting up the colour array
  Colour online_colour(0, 150, 20);
  STATUS_COLOURS[Online] = online_colour;
  Colour away_colour(200, 150, 0);
  STATUS_COLOURS[Away] = away_colour;
  Colour offline_colour(250, 0, 0);
  STATUS_COLOURS[Offline] = offline_colour;
  Colour on_break_colour(120, 0, 120);
  STATUS_COLOURS[On_Break] = on_break_colour;

  // Setup array of people
  for (int i = 0; i < PEOPLE_COUNT; i ++) {
    people[i].led = i;
    people[i].name = names[i];
    people[i].status = Status(i % STATUS_COUNT);
  }
}


void connectWIFI() {
  //This function is copied from the tutorial on Arduino.cc
  //about how to connect Arduino MKR WiFi 1010 to a network.
  
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


void printData() {
  //Also taken from Arduino.cc tutorial
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

//+++++++++++++++++ Things that are run in the loop +++++++++++++++++

void loop() {

  if(status_request_timer.hasPassed(120000)){ // Every two minutes, get status from server 
    Serial.println("Two minutes has passed - Requesting status from server");
    getStatus();
    status_request_timer.restart();
  }
  checkButtons();//Are any of the buttons being pressed?
  sort_set_status_requests();//If they have been pressed, send a set status request
  showColours();
  delay(200);
}

void checkButtons() {

  if (digitalRead(ONLINE_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    Serial.println("Online button pressed");
    online = true;
    away = false;
    on_break = false;
    offline = false;
    send_online = true;
  }

  if (digitalRead(AWAY_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    Serial.println("Away button pressed");
    online = false;
    away = true;
    on_break = false;
    offline = false;
    send_away = true;
  }

  if (digitalRead(BREAK_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    Serial.println("On Break button pressed");
    online = false;
    away = false;
    on_break = true;
    offline = false;
    send_on_break = true;
  }

  if (digitalRead(OFFLINE_BUTTON_PIN) == ON_ONE_OR_ZERO) {
    Serial.println("Offline button pressed");
    online = false;
    away = false;
    on_break = false;
    offline = true;
    send_offline = true;
  }

}

void sort_set_status_requests() {
  if (send_online == true) {
    send_online = false;
    getSetStatus("Online");//Send a set_status as Online.
    getStatus();// Retrieve most recent statuses
    Serial.println("Finished sending status");
    return;
  }
  if (send_away == true) {
    send_away = false;
    getSetStatus("Away");//Send the set_status as Away.
    getStatus();// Retrieve most recent statuses
    Serial.println("Finished sending status");
    return;
  }
  if (send_on_break == true) {
    send_on_break = false;
    getSetStatus("On_Break");//Send the set_status as On_Break.
    getStatus();// Retrieve most recent statuses
    Serial.println("Finished sending status");
    return;
  }
  if (send_offline == true) {
    send_offline = false;
    getSetStatus("Offline");//Send the set_status as Offline.
    getStatus();// Retrieve most recent statuses
    Serial.println("Finished sending status");
    return;
  }
}

void send_status_request() {
  String path = "/status";
  String HTTP_version = "1.0";
  Serial.println("Making HTTP Get request to /status");
  Serial.println("GET " + path + " HTTP/" + HTTP_version);//Show get request in console to compare
  client.println("GET " + path + " HTTP/" + HTTP_version);
  client.println("Host: " + String(HOST_NAME));
  client.println("Connection: close");
  client.println();
}

void set_status_request(String stat) {
  String path = "/set_status";
  String HTTP_version = "1.0";
  Serial.println("Setting your status as " + stat);
  Serial.println("GET " + path + my_name + "/" + stat + " HTTP/" + HTTP_version);// show get request in console
  client.println("GET " + path + my_name + "/" + stat + " HTTP/" + HTTP_version);
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
  const size_t capacity = 200;
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

void getSetStatus(String stat) {
  //sets up the client to send a set status request to the server.
  Serial.println("Attempting to set status");
  if (client.connect(HOST_NAME, 443)) {
    Serial.println("Connection to server successful");

    // Make the HTTPS set status request
    set_status_request(stat);
    delay(500);
    Serial.println("Closing connection, status has been set");
    client.stop();    
  }

}

void handle_status_update(DynamicJsonDocument status_dictionary) {
  // Check if a name is found in the dictionary
  if (status_dictionary.containsKey("Clare")) {
    // If it is, then let's get the status
    String clare_status = status_dictionary["Clare"];

    //Loop Through people and update their status from the Json string
    for (int i = 0; i < PEOPLE_COUNT; i++) {
      String current_name = people[i].name;
      if (status_dictionary.containsKey(current_name)) {
        String status_string = status_dictionary[current_name];
        people[i].status = enum_from_string(status_string);
        //Serial.println(people[i].name + "'s has been set to " + STATUS_STRINGS[people[i].status]);
      } else {
        Serial.println("Couldn't find " + current_name + " in the status Dictionary");
      }

    }
    set_colours();
    // Print it out and alert Clare that the JSON is ready for action!
    Serial.println("Ready to handle JSON, parsed successfully, Clare's status is: " + clare_status);
  }
  else {
    Serial.println("Couldn't find Clare in the status dictionary. Did something go wrong when parsing?");
  }
}


void showColours() {
  set_colours();
  strip.show();
  //delay(1000); //Removed second delay to allow program to run faster each loop.
}

void set_colours() {

  // Looping through people array and updating LEDs
  for (int i = 0; i < PEOPLE_COUNT; i++) {
    Person person = people[i];
    Colour c = STATUS_COLOURS[person.status];

    // Set corresponding pixel in strip
    // to the correct colour.
    strip.setPixelColor(person.led,
                        strip.Color(c.R,
                                    c.G,
                                    c.B)
                       );
  }
}


Status enum_from_string(String string_status) {
  //Cast the enum to a string - used to send the enum status to the server.
  if (string_status == "Online") {
    return Online;
  }
  if (string_status == "Away") {
    return Away;
  }
  if (string_status == "Offline") {
    return Offline;
  }
  if (string_status == "On_Break") {
    return On_Break;
  }
}
