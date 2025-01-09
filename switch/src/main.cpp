#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Arduino.h>
#include <RootHtml.h>

const char* ssid = "enl_timer_ap";
const char* password = "ZumBilderGrossMache";
#define RELAY_PIN 12
#define LED_PIN 13 // state is inverted: LOW->ON
// #define USE_LED

// Custom IP configuration
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
unsigned long timerStart = 0;
unsigned long timerDuration = 0;
bool timerActive = false;
bool relayState = false;

void handleRoot() {
  server.send(200, "text/html", rootHtml);
  Serial.println("root served");
}

void handleToggle() {
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  #ifdef USE_LED
    if (timerActive) digitalWrite(LED_PIN, HIGH);
  #endif
  timerActive = false;

  server.send(200, "text/plain", relayState ? "ON" : "OFF");
  Serial.println("Toggle");
}

void handleOn() {
  relayState = true;
  digitalWrite(RELAY_PIN, HIGH);

  #ifdef USE_LED
    if (timerActive) digitalWrite(LED_PIN, HIGH);
  #endif
  timerActive = false;

  server.send(200, "text/plain", "ON");
  Serial.println("On");
}

void handleOff() {
  relayState = false;
  digitalWrite(RELAY_PIN, LOW);

  #ifdef USE_LED
    if (timerActive) digitalWrite(LED_PIN, HIGH);
  #endif
  timerActive = false;

  server.send(200, "text/plain", "OFF");
  Serial.println("OFF");
}

void handleTimer() {
  if (server.hasArg("miliseconds")) {
    int miliseconds = server.arg("miliseconds").toInt();
    if (miliseconds > 0) {
      if (!relayState) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        #ifdef USE_LED
          digitalWrite(LED_PIN, LOW);
        #endif
        timerStart = millis();
        timerDuration = miliseconds;
        timerActive = true;
        server.send(200, "text/plain", "Timer started for " + String(miliseconds) + " miliseconds");
        Serial.println("Timer " + String(miliseconds));
      } else {
        server.send(200, "text/plain", "Already ON");
        Serial.println("Timer " + String(miliseconds) + " ignored because already ON");
      }
    } else {
      server.send(400, "text/plain", "Invalid duration");
    }
  } else {
    server.send(400, "text/plain", "Missing miliseconds parameter");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure AP with custom IP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  pinMode(RELAY_PIN, OUTPUT); 
  digitalWrite(RELAY_PIN, relayState); 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // off by default

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/timer", handleTimer);
  server.begin();
  Serial.println("HTTP server started");


}

void loop() {
  server.handleClient();
  if (timerActive && millis() - timerStart > timerDuration) {
    timerActive = false;
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    #ifdef USE_LED
      digitalWrite(LED_PIN, HIGH);
    #endif
  }
}
