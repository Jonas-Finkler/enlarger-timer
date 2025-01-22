#include <Arduino.h>
#include <Encoder.h>
#include <Button.h>
#include <Switch.h>
#include <TM1637Display.h>


#define ENCODER_SW_PIN D6
#define ENCODER_DT_PIN D5
#define ENCODER_CLK_PIN D4
#define LED_DIO D1
#define LED_CLK D2
#define BUTTON_L_PIN D3
#define BUTTON_R_PIN D7 

const uint8_t SEG_ON[] = {
  0,
	SEG_E | SEG_G | SEG_C | SEG_D,   // o
	SEG_C | SEG_E | SEG_G,           // n
  0,
	};
const uint8_t SEG__ON_[] = {
  SEG_D,                           // _ 
	SEG_E | SEG_G | SEG_C | SEG_D,   // o
	SEG_C | SEG_E | SEG_G,           // n
  SEG_D,                           // _
	};
const uint8_t SEG_CONN[] = {
	SEG_E | SEG_G | SEG_D,           // c
	SEG_E | SEG_G | SEG_C | SEG_D,   // o
	SEG_C | SEG_E | SEG_G,           // n
	SEG_C | SEG_E | SEG_G,           // n
	};
TM1637Display ledDisplay(LED_CLK, LED_DIO);


// TODO: 
// - the left button should start a 30s timer without turning on the lamp
// - when held donw, the left button should allow to set the blind timer

 
volatile bool leftButtonWasPressed = false;
void leftButtonPressed() { leftButtonWasPressed = true; }
Button leftButton(BUTTON_L_PIN, leftButtonPressed);

volatile bool encoderButtonWasPressed = false;
void encoderButtonPressed() { encoderButtonWasPressed = true; }
Button encoderButton(ENCODER_SW_PIN, encoderButtonPressed);

volatile bool buttonWasPressed = false;
void buttonPressed() { buttonWasPressed = true; }
Button button(BUTTON_R_PIN, buttonPressed);

Encoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);
unsigned long lastEncoderChange = 0;
int lastEncoderValue = 0;
int encoderRefValue = 0;
int encoderAccum = 0;

int setTime = 0;

unsigned long timerStart = 0;
bool timerRunning = false;
int timerDuration = 0;

unsigned long lastSwitchChange = 0;
bool switchState = false;

bool onMode = false;
bool fastMode = false;


void updateEncoder() {
  if (onMode) { // ignore all changes (delta has been set to zero)
    encoderRefValue = encoder.read();
    return;
  }
  int encoderValue = encoder.read();
  if (encoderValue != lastEncoderValue) {
    lastEncoderChange = millis();
    lastEncoderValue = encoderValue;
  }
  /*encoderRefValue = min(encoderValue, encoderRefValue);*/
  int encoderDelta = encoderValue - encoderRefValue;
  int setTimeDelta = (encoderDelta + 2 + 40) / 4 - 10; // adding the 40 to always round down 
  if (fastMode) {
    setTimeDelta *= 5;
  }
  setTime = setTimeDelta + encoderAccum ; // in 1/10 s

  // NOTE: implements: setTime = max(-1, setTime); but avoids hidden negative encoderDelta
  if (fastMode) {
    encoderAccum = max(-1 * setTimeDelta - 5, encoderAccum);
  } else {
    encoderAccum = max(-1 * setTimeDelta - 1, encoderAccum);
  }

  // adjust reference, in case encoder gets out of alignment (4 per click)
  if ((millis() - lastEncoderChange > 500) && !timerRunning && encoderDelta != 0) {
    encoderAccum = setTime;
    encoderRefValue = encoderValue;
  }
}

void setup() {
  Serial.begin(115200);
  encoderRefValue = encoder.read();
  lastEncoderValue = encoderRefValue;
  ledDisplay.setBrightness(0, true);
}

void loop() {
  /*unsigned long loopStart = millis();*/
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, trying to connect");
    ledDisplay.setSegments(SEG_CONN);
    connectWiFi();
    switchState = getState();
    return;
  }
  
  updateEncoder();

  if (buttonWasPressed) {
    buttonWasPressed = false;
    if (timerRunning) { // cancel timer
      timerRunning = false;
      setState(false);
    } else {
      if (setTime > 0 && !onMode) {
        timerDuration = setTime * 100;
        setTimer(timerDuration);
        timerStart = millis();
        timerRunning = true;
      }
    }
  }

  if (leftButtonWasPressed) {
    leftButtonWasPressed = false;
    if (!timerRunning) {
      if (setTime < 0) { // reset to 0, turn off
        encoderAccum = 0;
        encoderRefValue = encoder.read();
        onMode = false;
      } else {
        if (!onMode) { // freeze setTime
          encoderAccum = setTime;
          encoderRefValue = encoder.read();
        }
        onMode = !onMode;
      }
    } 
  }

  if (encoderButtonWasPressed) {
    encoderButtonWasPressed = false;
    fastMode = !fastMode;
    encoderAccum = setTime;
    encoderRefValue = encoder.read();
  }

  bool shouldBeOn = setTime < 0 || onMode;
  // update switch state if necessary
  if (shouldBeOn != switchState) {
    if (millis() - lastSwitchChange > 500) {
      switchState = shouldBeOn;
      setState(switchState);
      lastSwitchChange = millis();
    }
  }

  // display stuff
  if (timerRunning) {
    int timeLeft = timerDuration - (millis() - timerStart);
    if (timeLeft <= 0) {
      timerRunning = false;
      timerDuration = 0;
      timerStart = 0;
    }
    /*Serial.println("Time left: " + String(timeLeft / 1000.f));*/
    ledDisplay.showNumberDecEx(timeLeft / 100, 0b00100000, true);
  } else {
    if (shouldBeOn) {
      /*Serial.println("Switch is on");*/
      if (setTime < 0) { // indicate why it is on
        ledDisplay.setSegments(SEG__ON_);
      } else {
        ledDisplay.setSegments(SEG_ON);
      }
    } else {
      /*Serial.println("Set time: " + String(setTime / 10.f) + " s");*/
      ledDisplay.showNumberDecEx(setTime, 0b00100000, true);
    }
  }

  /*unsigned long loopDuration = millis() - loopStart;*/
  /*Serial.println("Loop duration: " + String(loopDuration) + " ms");*/

}
