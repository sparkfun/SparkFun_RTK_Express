void checkButtons()
{
  //Check power button
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime == 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(pin_powerSenseAndControl) == LOW)
    {
      powerPressedStartTime = millis();
    }
  }
  else if (digitalRead(pin_powerSenseAndControl) == LOW && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(pin_powerSenseAndControl) == LOW)
    {
      if ((millis() - powerPressedStartTime) > 2000)
      {
        powerDown(true);
      }
    }
  }
  else if (digitalRead(pin_powerSenseAndControl) == HIGH && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(pin_powerSenseAndControl) == HIGH)
    {
      Serial.print("Power button released after ms: ");
      Serial.println(millis() - powerPressedStartTime);
      powerPressedStartTime = 0; //Reset var to return to normal 'on' state
    }
  }
  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

}

//User has pressed the power button to turn on the system
//Was it an accidental bump or do they really want to turn on?
//Let's make sure they continue to press for two seconds
void powerOnCheck()
{
  powerPressedStartTime = millis();
  while (digitalRead(pin_powerSenseAndControl) == LOW)
  {
    delay(100); //Wait for user to stop pressing button.

    if (millis() - powerPressedStartTime > 500)
      break;
  }

  if (millis() - powerPressedStartTime < 500)
    powerDown(false); //Power button tap. Returning to off state.

  powerPressedStartTime = 0; //Reset var to return to normal 'on' state
}

//If we have a power button tap, or if the display is not yet started (no I2C!)
//then don't display a shutdown screen
void powerDown(bool displayInfo)
{
  //Close down file system
//  eventsFile.sync();
//  eventsFile.close();

  if (displayInfo == true)
  {
    displayShutdown();
    delay(2000);
  }

  pinMode(pin_powerSenseAndControl, OUTPUT);
  digitalWrite(pin_powerSenseAndControl, LOW);

  pinMode(pin_powerFastOff, OUTPUT);
  digitalWrite(pin_powerFastOff, LOW);

  while (1)
    delay(1);
}
