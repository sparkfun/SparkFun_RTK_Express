//Check setup button and move between base and rover states
void checkSetupButton()
{
  //Check setup button and configure module accordingly
  if (digitalRead(setupButton) == LOW && setupButtonRecorded == false)
  {
    delay(20); //Debounce
    if (digitalRead(setupButton) == LOW)
    {
      setupButtonRecorded = true;

      if (baseState == BASE_OFF)
      {
        //Configure for base mode
        Serial.println(F("Base Mode"));

        if (configureUbloxModuleBase() == false)
        {
          Serial.println(F("Base config failed!"));
          return;
        }

        baseState = BASE_SURVEYING_IN_NOTSTARTED; //Switch to new state
        currentScreen = SCREEN_BASE_SURVEYING_NOTSTARTED;

        //Restart Bluetooth with 'Base' name
        //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
        beginBluetooth();
      }
      else if (baseState == BASE_SURVEYING_IN_NOTSTARTED ||
               baseState == BASE_SURVEYING_IN_SLOW ||
               baseState == BASE_SURVEYING_IN_FAST ||
               baseState == BASE_TRANSMITTING)
      {
        //Configure for rover mode
        Serial.println(F("Rover Mode"));

        //If we are survey'd in, but switch is rover then disable survey
        if (configureUbloxModuleRover() == false)
        {
          Serial.println(F("Rover config failed!"));
          return;
        }

        baseState = BASE_OFF;
        currentScreen = SCREEN_ROVER;

        beginBluetooth(); //Restart Bluetooth with 'Rover' name
      }
    }
  }
  else if (digitalRead(setupButton) == HIGH && setupButtonRecorded == true)
  {
    setupButtonRecorded = false;
  }
}

void powerOnCheck()
{
  //User has pressed the power button to turn on the system
  //Was it an accidental bump or do they really want to turn on?
  //Let's make sure they continue to press for two seconds
  powerPressedStartTime = millis();
  while (digitalRead(powerSenseAndControl) == LOW)
  {
    delay(100); //Wait for user to stop pressing button.

    if (millis() - powerPressedStartTime > 500)
      break;
  }

  if (millis() - powerPressedStartTime < 500)
    powerDown(); //Power button tap. Returning to off state.

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
      Serial.println("User is pressing power button. Start timer.");
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
        Serial.println("Time to power down!");
        powerDown();
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
    }
    powerPressedStartTime = 0; //Reset var to return to normal 'on' state
  }
}

void powerDown()
{
  displayShutdown();
  delay(2000);

  pinMode(powerSenseAndControl, OUTPUT);
  digitalWrite(powerSenseAndControl, LOW);

  pinMode(powerFastOff, OUTPUT);
  digitalWrite(powerFastOff, LOW);

  while (1)
    delay(1);
}
