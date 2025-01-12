#include <Arduino.h>
#include <Encoder.h>
#include <Button.h>
#include <Switch.h>

#define ENCODER_SW_PIN D7
#define ENCODER_DT_PIN D6
#define ENCODER_CLK_PIN D5

void encoderButtonPressed();


Button encoderButton(ENCODER_SW_PIN, encoderButtonPressed);
Encoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);
unsigned long lastEncoderChange = 0;
int lastEncoderValue = 0;
int encoderRefValue = 0;
unsigned long timerStart = 0;
bool timerRunning = false;
int timerDuration = 0;

unsigned long lastSwitchChange = 0;
bool switchState = false;

volatile bool encoderWasPressed = false;

void encoderButtonPressed() {
  encoderWasPressed = true;
}

void setup() {
  Serial.begin(115200);
  encoderRefValue = encoder.read();
  lastEncoderValue = encoderRefValue;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, trying to connect");
    connectWiFi();
    switchState = getState();
    return;
  }
  
  int encoderValue = encoder.read();
  if (encoderValue != lastEncoderValue) {
    lastEncoderChange = millis();
    lastEncoderValue = encoderValue;
  }
  /*encoderRefValue = min(encoderValue, encoderRefValue);*/
  int encoderDelta = encoderValue - encoderRefValue;
  if (encoderDelta < -4) {
    encoderRefValue = encoderValue + 4;
    encoderDelta = -4;
  }
  int setTime = (encoderDelta + 2) / 4 * 1; // in 1/10 s
  setTime = max(0, setTime);

  // adjust reference, in case encoder gets out of alignment (4 per click)
  if ((millis() - lastEncoderChange > 2000) && !timerRunning) {
    int idealEncoderDelta = ((encoderDelta + 2 + 40) / 4) * 4 - 40;
    encoderRefValue = encoderValue - idealEncoderDelta;
  }



  if ((encoderDelta < 0) != switchState) {
    if (millis() - lastSwitchChange > 500) {
      switchState = encoderDelta < 0;
      setState(switchState);
      lastSwitchChange = millis();
    }
  }



  if (encoderWasPressed) {
    encoderWasPressed = false;
    if (setTime > 0) {
      setTimer(setTime * 100);
      timerStart = millis();
      timerRunning = true;
      timerDuration = setTime * 100;
    }
  }

  if (timerRunning) {
    int timeLeft = timerDuration - (millis() - timerStart);
    if (timeLeft <= 0) {
      timerRunning = false;
      timerDuration = 0;
      timerStart = 0;
    }
    Serial.println("Time left: " + String(timeLeft / 1000.f));
  } else {
    if (encoderDelta < 0) {
      Serial.println("Switch is on");
    } else {
      Serial.println("Set time: " + String(setTime / 10.f) + " s");
    }
  }

  /*Serial.println("setTime: " + String(setTime / 10.f) + "  " + String(encoderValue - encoderRefValue) + "  "  + String(encoderRefValue));*/

  // NOTE: Remove this later
  delay(50);

}
