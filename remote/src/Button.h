#include <Arduino.h>


class Button{
private:
  int pin;
  void (*callback)(void);
  const int debounceDelay = 500;
  volatile unsigned long lastButtonPress = 0;

  void buttonPressed(){
    if (millis() - lastButtonPress > debounceDelay){
      lastButtonPress = millis();
      callback();
    }
  }

public:
  static void IRAM_ATTR forwardButtonPressed(void *instance){
    ((Button*)instance)->buttonPressed();
  }

  Button(int pin, void (*callback)(void)){
    this->pin = pin;
    this->callback = callback;
    pinMode(pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(pin), forwardButtonPressed, this, FALLING);
  }

};
