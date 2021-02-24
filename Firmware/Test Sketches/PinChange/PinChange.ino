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


void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Pin Change with debounce example");

  pinMode(setupButton, INPUT_PULLUP);
}

void loop()
{
  checkSetupButton();

  delay(10);
}

bool setupButtonRecorded = false;

void checkSetupButton()
{
  if (digitalRead(setupButton) == LOW && setupButtonRecorded == false)
  {
    delay(20); //Debounce
    if (digitalRead(setupButton) == LOW)
    {
      setupButtonRecorded = true;
      
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
  }
  else if(digitalRead(setupButton) == HIGH && setupButtonRecorded == true)
    setupButtonRecorded = false;
}
