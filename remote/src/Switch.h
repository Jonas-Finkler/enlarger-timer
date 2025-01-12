#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


const char* ssid = "enl_timer_ap";
const char* password = "ZumBilderGrossMache";
IPAddress switchIP(192, 168, 1, 1);


WiFiClient client;
HTTPClient http;


void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

bool getState() {
  bool state = false;
  String url = "http://" + switchIP.toString() + "/state";

  http.begin(client, url);
  int httpCode = http.GET();
  Serial.println("HTTP Response code: " + String(httpCode));

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Response payload: " + payload);
    state = payload == "ON";
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();
  return state;
}

void setState(bool state) {
  String stateString = state ? "on" : "off";
  String url = "http://" + switchIP.toString() + "/" + stateString;
  http.begin(client, url);
  int httpCode = http.GET();
  Serial.println("HTTP Response code: " + String(httpCode));
  http.end();
}

void setTimer(unsigned long millis) {
  String url = "http://" + switchIP.toString() + "/timer?miliseconds=" + String(millis);
  http.begin(client, url);
  int httpCode = http.GET();
  Serial.println("HTTP Response code: " + String(httpCode));
  http.end();
}
