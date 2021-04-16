//User has pressed the power button to turn on the system
//Was it an accidental bump or do they really want to turn on?
//Let's make sure they continue to press for two seconds
void powerOnCheck()
{
  //pinMode(powerSenseAndControl, INPUT_PULLUP);
  pinMode(powerSenseAndControl, INPUT);
  //pinMode(powerFastOff, INPUT);

  powerPressedStartTime = millis();
  while (digitalRead(powerSenseAndControl) == LOW)
  {
    //Serial.print("!");
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
    //accelEnd();

  accel.setInt1(true);
  accel.setInt1Latch(true);
    accel.setDataRate(LIS2DH12_ODR_1Hz);
    accel.setInt1Duration(1);
    //accel.setDataRate(LIS2DH12_ODR_10Hz); //Works

    displayShutdown();
    delay(2000);
  }

  //esp_sleep_enable_ext0_wakeup(accelInt, LOW)

  pinMode(peripheralPower, OUTPUT);
  digitalWrite(peripheralPower, LOW);

  pinMode(powerSenseAndControl, OUTPUT);
  digitalWrite(powerSenseAndControl, LOW);

  pinMode(powerFastOff, OUTPUT);
  digitalWrite(powerFastOff, LOW);

  //digitalWrite(statLED, LOW);

  while (1)
    delay(1);
}

//Config accel for complete power off
//This avoids pulling the ACCEL_INT pin low on ESP causing power circuit issues
//Assume wire and accel has been started
void accelEnd()
{
  accel.setMode(LIS2DH12_LP_8bit);
  accel.setDataRate(LIS2DH12_POWER_DOWN); //Stop measurements

  accel.disableTemperature();
  accel.setInt1(false);
  accel.setInt1IA1(false);

  //accel.setIntPolarity(HIGH);
  accel.setInt1Latch(false);
  accel.setInt1Threshold(0);
  accel.setInt1Duration(0);
  while (accel.getInt1()) delay(10); //Reading int will clear it
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
