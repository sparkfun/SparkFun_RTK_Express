//Check setup button and move between base and rover configurations
//Since the setup button is the only state change the user can directly affect
//we do the state changes here rather than in updateSystemState()
void checkSetupButton()
{
  //Check setup button and configure module accordingly
  if (digitalRead(setupButton) == LOW || setupByPowerButton == true)
  {
    delay(20); //Debounce
    if (digitalRead(setupButton) == LOW || setupByPowerButton == true)
    {
      setupByPowerButton = false;

      if (systemState == STATE_ROVER_NO_FIX ||
          systemState == STATE_ROVER_FIX ||
          systemState == STATE_ROVER_RTK_FLOAT ||
          systemState == STATE_ROVER_RTK_FIX)
      {
        displayBaseStart();

        //Configure for base mode
        Serial.println(F("Base Mode"));


        //Restart Bluetooth with 'Base' name
        //We start BT regardless of Ntrip Server in case user wants to transmit survey-in stats over BT
        beginBluetooth();

        if (settings.fixedBase == false)
        {
          //Don't configure base if we are going to do a survey in. We need to wait for surveyInStartingAccuracy to be achieved in state machine
          systemState = STATE_BASE_TEMP_SURVEY_NOT_STARTED;
        }
        else if (settings.fixedBase == true)
        {
          if (configureUbloxModuleBase() == false)
          {
            Serial.println(F("Base config failed!"));
            displayBaseFail();
            return;
          }

          bool response = startFixedBase();
          if (response == true)
          {
            systemState = STATE_BASE_FIXED_TRANSMITTING;
          }
          else
          {
            //TODO maybe create a custom fixed base fail screen
            Serial.println(F("Fixed base start failed!"));
            displayBaseFail();
            return;
          }
        }

        displayBaseSuccess();
        delay(500);
      }
      else if (systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED ||
               systemState == STATE_BASE_TEMP_SURVEY_STARTED ||
               systemState == STATE_BASE_TEMP_TRANSMITTING ||
               systemState == STATE_BASE_TEMP_WIFI_STARTED ||
               systemState == STATE_BASE_TEMP_WIFI_CONNECTED ||
               systemState == STATE_BASE_TEMP_CASTER_STARTED ||
               systemState == STATE_BASE_TEMP_CASTER_CONNECTED ||
               systemState == STATE_BASE_FIXED_TRANSMITTING ||
               systemState == STATE_BASE_FIXED_WIFI_STARTED ||
               systemState == STATE_BASE_FIXED_WIFI_CONNECTED ||
               systemState == STATE_BASE_FIXED_CASTER_STARTED ||
               systemState == STATE_BASE_FIXED_CASTER_CONNECTED)
      {
        displayRoverStart();

        //Configure for rover mode
        Serial.println(F("Rover Mode"));

        //If we are survey'd in, but switch is rover then disable survey
        if (configureUbloxModuleRover() == false)
        {
          Serial.println(F("Rover config failed!"));
          displayRoverFail();
          return;
        }

        beginBluetooth(); //Restart Bluetooth with 'Rover' name

        systemState = STATE_ROVER_NO_FIX;
        displayRoverSuccess();
        delay(500);
      }
    }
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
