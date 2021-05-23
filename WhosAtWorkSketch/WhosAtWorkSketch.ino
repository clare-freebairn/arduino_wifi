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
char HOST_NAME[] = "https://who-s-at-work-a7zmhe11fk5c.runkit.sh";
String HTTP_METHOD = "GET"; // or POST
String SET_METHOD = "/set_status";
String STATUS_METHOD = "/status";
String LISTEN_METHOD = "/status_changed";

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
  sendStatus();
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

void sendStatus() {
  //Unfinished
  if (send_online == true) {
    send_online = false;

    if (client.connect("who-s-at-work-a7zmhe11fk5c.runkit.sh", 443)) {
      Serial.println("Connected to server ");
      client.println(HTTP_METHOD + " " + "/status" + " HTTP/1.1");
      client.println("Host: " + String("who-s-at-work-a7zmhe11fk5c.runkit.sh"));
      client.println("Connection: close");
      client.println();
      while (client.connected()) {
        if (client.available()) {

          // read an incoming byte from the server and print it to serial monitor:
          char c = client.read();
          Serial.print(c);
        }
      }
      // the server's disconnected, stop the client:
      client.stop();
      Serial.println();
      Serial.println("disconnected");

    } else {
      Serial.println("Not connected to server :-(");
    }
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
