//User has pressed the power button to turn on the system
//Was it an accidental bump or do they really want to turn on?
//Let's make sure they continue to press for two seconds
void powerOnCheck()
{
  powerPressedStartTime = millis();
  while (digitalRead(powerSenseAndControl) == LOW)
  {
    delay(100); //Wait for user to stop pressing button.

    if (millis() - powerPressedStartTime > 500)
      break;
  }

  if (millis() - powerPressedStartTime < 500)
    powerDown(false); //Power button tap. Returning to off state.

  powerPressedStartTime = 0; //Reset var to return to normal 'on' state
}

void checkPowerButton()
{
  if (digitalRead(powerSenseAndControl) == LOW && powerPressedStartTime == 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(powerSenseAndControl) == LOW)
    {
      powerPressedStartTime = millis();
    }
  }
  else if (digitalRead(powerSenseAndControl) == LOW && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(powerSenseAndControl) == LOW)
    {
      if ((millis() - powerPressedStartTime) > 2000)
      {
        powerDown(true);
      }
    }
  }
  else if (digitalRead(powerSenseAndControl) == HIGH && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(powerSenseAndControl) == HIGH)
    {
      Serial.print("Power button released after ms: ");
      Serial.println(millis() - powerPressedStartTime);
      powerPressedStartTime = 0; //Reset var to return to normal 'on' state

      setupByPowerButton = true; //Notify checkSetupButton()
    }
  }
}

//If we have a power button tap, or if the display is not yet started (no I2C!)
//then don't display a shutdown screen
void powerDown(bool displayInfo)
{
  if (displayInfo == true)
  {
    displayShutdown();
    delay(2000);
  }

  pinMode(powerSenseAndControl, OUTPUT);
  digitalWrite(powerSenseAndControl, LOW);

  pinMode(powerFastOff, OUTPUT);
  digitalWrite(powerFastOff, LOW);

  while (1)
    delay(1);
}

void displayShutdown()
{
  //Show shutdown text
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(21, 13);
  oled.setFontType(1);

  int textX = 2;
  int textY = 10;
  int textKerning = 8;

  printTextwithKerning("Shutting", textX, textY, textKerning);

  textX = 4;
  textY = 25;
  textKerning = 9;
  oled.setFontType(1);

  printTextwithKerning("Down...", textX, textY, textKerning);

  oled.display();
}
