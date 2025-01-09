#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Encoder.h>

#define ENCODER_SW_PIN D7
#define ENCODER_DT_PIN D6
#define ENCODER_CLK_PIN D5

const char* ssid = "enl_timer_ap";
const char* password = "ZumBilderGrossMache";
IPAddress switchIP(192, 168, 1, 1);

WiFiClient client;
HTTPClient http;

Encoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);
int encoderRefValue = 0;

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

volatile bool encoderWasPressed = false;
volatile unsigned long lastEncoderPress = 0;

void IRAM_ATTR encoderPressed() {
  if (millis() - lastEncoderPress < 1000) {
    return;
  }
  encoderWasPressed = true;
  lastEncoderPress = millis();
}

void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_SW_PIN), encoderPressed, FALLING);
  encoderRefValue = encoder.read();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, trying to connect");
    connectWiFi();
    return;
  }

  /*bool state = getState();*/
  /*Serial.println("state: " + String(state));*/



  /*String url = "http://" + switchIP.toString() + "/toggle";*/
  /**/
  /*http.begin(client, url);*/
  /*int httpCode = http.GET();*/
  /*Serial.println("HTTP Response code: " + String(httpCode));*/
  /**/
  /*if (httpCode > 0) {*/
  /*  String payload = http.getString();*/
  /*  Serial.println("Response payload: " + payload);*/
  /*} else {*/
  /*  Serial.println("Error on HTTP request");*/
  /*}*/
  /**/
  /*http.end();*/
  
  int encoderValue = encoder.read();
  encoderRefValue = min(encoderValue, encoderRefValue);
  int setTime = (encoderValue - encoderRefValue) / 4 * 1; // in 1/10 s
  Serial.println("setTime: " + String(setTime / 10.f));

  if (encoderWasPressed) {
    encoderWasPressed = false;
    if (setTime > 0) {
      setTimer(setTime * 100);
    }
    /*bool state = getState();*/
    /*Serial.println("state: " + String(state));*/
    /*setState(!state);*/
    /*Serial.println("state: " + String(!state));*/
  }

}
