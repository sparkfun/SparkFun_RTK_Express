/*
  Change Base/Rover state when user presses setup button
*/

//v16
int POWER_CTRL = 13;
int FAST_OFF = 27;
int STAT_LED = 12;
const int setupButton = 14;

long powerPressedStartTime = 0;
int debounceDelay = 20;

//bool setupButtonPressed = false;

#include "settings.h"

volatile int numberOfButtonInterrupts = 0;
volatile bool lastState;
volatile uint32_t debounceTimeout = 0;

// For setting up critical sections, used to disable and interrupt interrupts
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR changeSetupModes()
{
  portENTER_CRITICAL_ISR(&mux);
  numberOfButtonInterrupts++;
  lastState = digitalRead(setupButton);
  debounceTimeout = xTaskGetTickCount(); //version of millis() that works from interrupt
  portEXIT_CRITICAL_ISR(&mux);
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Pin Change with debounce example");

  pinMode(setupButton, INPUT_PULLUP);
  attachInterrupt(setupButton, changeSetupModes, FALLING);
}

void loop()
{
  checkSetupButton();

  //delay(10);
}

void checkSetupButton()
{
  uint32_t saveDebounceTimeout;
  bool saveLastState;
  int save;

  portENTER_CRITICAL_ISR(&mux); // so that value of numberOfButtonInterrupts,l astState are atomic - Critical Section
  save = numberOfButtonInterrupts;
  saveDebounceTimeout = debounceTimeout;
  saveLastState = lastState;
  portEXIT_CRITICAL_ISR(&mux); // end of Critical Section

  bool currentState = digitalRead(setupButton);

  //https://www.switchdoc.com/2018/04/esp32-tutorial-debouncing-a-button-press-using-interrupts/
  // if Interrupt Has triggered AND Button Pin is in same state AND the debounce time has expired THEN you have the button push!
  if ((save != 0) //interrupt has triggered
      && (currentState == saveLastState) // pin is still in the same state as when intr triggered
      && (millis() - saveDebounceTimeout > debounceDelay ))
  { // and it has been low for at least DEBOUNCETIME, then valid keypress

    if (currentState == LOW)
    {
      if (baseState == BASE_OFF)
      {
        baseState = BASE_SURVEYING_IN_NOTSTARTED;
        Serial.println("Base started");
      }
      else if (baseState == BASE_SURVEYING_IN_NOTSTARTED)
      {
        baseState = BASE_OFF;
        Serial.println("Base off");
      }
    }

    portENTER_CRITICAL_ISR(&mux); // can't change it unless, atomic - Critical section
    numberOfButtonInterrupts = 0; // acknowledge keypress and reset interrupt counter
    portEXIT_CRITICAL_ISR(&mux);
  }
}
