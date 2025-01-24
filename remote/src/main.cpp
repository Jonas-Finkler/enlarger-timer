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



// --- buttons --- 
volatile bool leftButtonWasPressed = false;
void leftButtonPressed() { leftButtonWasPressed = true; }
Button leftButton(BUTTON_L_PIN, leftButtonPressed);

volatile bool encoderButtonWasPressed = false;
void encoderButtonPressed() { encoderButtonWasPressed = true; }
Button encoderButton(ENCODER_SW_PIN, encoderButtonPressed);

volatile bool buttonWasPressed = false;
void buttonPressed() { buttonWasPressed = true; }
Button button(BUTTON_R_PIN, buttonPressed);

// --- encoder ---
Encoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN);
unsigned long lastEncoderChange = 0;
int lastEncoderValue = 0;
int encoderRefValue = 0;
int encoderAccum = 0;

// --- timer ---
unsigned long timerDoneTime = 0;
unsigned long silentTimerDoneTime = 0;

// --- ui ---
int setTime = 0; // displayed time
bool fastMode = true; // 5x speed for encoder time setting

// --- wifi switch ---
unsigned long lastSwitchChange = 0;
bool switchState = false;



long timerRemaining(unsigned long timerDoneTime) {
  return timerDoneTime - millis();
}

void updateEncoder() {
  int encoderValue = encoder.read();

  if (encoderValue != lastEncoderValue) { 
    lastEncoderChange = millis();
    lastEncoderValue = encoderValue;
  }

  int encoderDelta = encoderValue - encoderRefValue;
  int setTimeDelta = (encoderDelta + 2 + 40) / 4 - 10; // adding the 40 to always round down 
  if (fastMode) {
    setTimeDelta *= 5;
  }
  setTime = setTimeDelta + encoderAccum ; // in 1/10 s

  // negative set time turns the switch on
  // this avoids the set time going too low, limit is -1 or -5 in fast mode
  if (fastMode) {
    if (setTime < 0) {
      encoderAccum = (setTime + 1) % 5 - 1 - setTimeDelta;
    }
  } else {
    encoderAccum = max(-1 * setTimeDelta - 1, encoderAccum);
  }

  // adjust reference, in case encoder gets out of alignment (4 per click)
  if ((millis() - lastEncoderChange > 500) && encoderDelta != 0) {
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

// time is in ms
void displayTime(int time) { 
  if (time >= 1000) {
    ledDisplay.showNumberDecEx(time / 100, 0b00100000, false);
  } else {
    ledDisplay.showNumberDecEx(0, 0b00100000, false, 3, 0); // __0.
    ledDisplay.showNumberDecEx(time / 100, 0b000000, true, 1, 3); // x
  }
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

  long remainingTimerTime = timerRemaining(timerDoneTime);
  long remainingSilentTimerTime = timerRemaining(silentTimerDoneTime);

  // cancel silent timer if running, otherwise start or cancel normal timer
  if (buttonWasPressed) {
    buttonWasPressed = false;

    if (remainingSilentTimerTime > 0) { // cancel silent timer
      silentTimerDoneTime = millis() - 1;
    } else {
      if (remainingTimerTime > 0) { // cancel timer
        timerDoneTime = millis() - 1;
        setState(false);
      } else {
        if (setTime > 0) { // start timer
          int timerDuration = setTime * 100;
          setTimer(timerDuration);
          timerDoneTime = millis() + timerDuration;
        }
      }
    }
  }

  // add 30s to silent timer
  if (leftButtonWasPressed) {
    leftButtonWasPressed = false;

    if (remainingSilentTimerTime < 0) {
      silentTimerDoneTime = millis();
    }
    silentTimerDoneTime += 30 * 1000;
  }

  // toggle fast mode
  if (encoderButtonWasPressed) {
    encoderButtonWasPressed = false;
    fastMode = !fastMode;
    encoderAccum = setTime;
    encoderRefValue = encoder.read();
  }

  bool shouldBeOn = setTime < 0;
  // update switch state if necessary
  if (shouldBeOn != switchState) {
    if (millis() - lastSwitchChange > 100) {
      switchState = shouldBeOn;
      setState(switchState);
      lastSwitchChange = millis();
    }
  }

  // display stuff
  if (remainingSilentTimerTime > 0) { 
      /*Serial.println("Silent time left: " + String(remainingSilentTimerTime / 1000.f));*/
    displayTime(remainingSilentTimerTime);
  } else {
    if (remainingTimerTime > 0) {
      /*Serial.println("Time left: " + String(remainingTimerTime / 1000.f));*/
      displayTime(remainingTimerTime);
    } else {
      if (shouldBeOn) {
        /*Serial.println("Switch is on");*/
        ledDisplay.setSegments(SEG_ON);
      } else {
        /*Serial.println("Set time: " + String(setTime / 10.f) + " s");*/
        displayTime(setTime * 100);
      }
    }
  }

  /*unsigned long loopDuration = millis() - loopStart;*/
  /*Serial.println("Loop duration: " + String(loopDuration) + " ms");*/

}
