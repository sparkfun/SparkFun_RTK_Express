void updateDisplay()
{
  //Update the display if connected
  if (online.display == true)
  {
    if (millis() - lastDisplayUpdate > 500) //Update display at 2Hz
    {
      lastDisplayUpdate = millis();

      oled.clear(PAGE); // Clear the display's internal buffer

      paintBatteryLevel(); //Top right corner

      paintWirelessIcon(); //Top left corner

      paintBaseState(); //Top center

      if (currentScreen == SCREEN_ROVER)
        paintRover();
      else if (currentScreen == SCREEN_ROVER_RTCM)
        paintRoverRTCM();
      else if (currentScreen == SCREEN_BASE_SURVEYING_NOTSTARTED)
        paintBaseSurveyNotStarted(); //Blink crosshair
      else if (currentScreen == SCREEN_BASE_SURVEYING_STARTED)
        paintBaseSurveyStarted(); //Blink base icon
      else if (currentScreen == SCREEN_BASE_TRANSMITTING)
        paintBaseTransmitting();
      else if (currentScreen == SCREEN_BASE_FAILED)
        paintBaseFailed();

      oled.display(); //Push internal buffer to display
    }
  }
}

void displaySplash()
{
  //Init and display splash
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(10, 2); //x, y
  oled.setFontType(0); //Set font to smallest
  oled.print(F("SparkFun"));

  oled.setCursor(21, 13);
  oled.setFontType(1);
  oled.print(F("RTK"));

  int textX = 3;
  int textY = 25;
  int textKerning = 9;
  oled.setFontType(1);
  printTextwithKerning("Express", textX, textY, textKerning);

  oled.setCursor(20, 41);
  oled.setFontType(0); //Set font to smallest
  oled.printf("v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
  oled.display();
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

//Displays a small error message then hard freeze
//Text wraps and is small but legible
void displayError(char * errorMessage)
{
    oled.clear(PAGE); // Clear the display's internal buffer

    oled.setCursor(0, 0); //x, y
    oled.setFontType(0); //Set font to smallest
    oled.print(F("Error:"));

    oled.setCursor(2, 10);
    //oled.setFontType(1);
    oled.print(errorMessage);

    oled.display(); //Push internal buffer to display

    while(1) delay(10); //Hard freeze
}

//Print the classic battery icon with levels
void paintBatteryLevel()
{
  //Current battery charge level
  if (battLevel < 25)
    oled.drawIcon(45, 0, Battery_0_Width, Battery_0_Height, Battery_0, sizeof(Battery_0), true);
  else if (battLevel < 50)
    oled.drawIcon(45, 0, Battery_1_Width, Battery_1_Height, Battery_1, sizeof(Battery_1), true);
  else if (battLevel < 75)
    oled.drawIcon(45, 0, Battery_2_Width, Battery_2_Height, Battery_2, sizeof(Battery_2), true);
  else //batt level > 75
    oled.drawIcon(45, 0, Battery_3_Width, Battery_3_Height, Battery_3, sizeof(Battery_3), true);
}

//Display Bluetooth icon, Bluetooth MAC, or WiFi depending on connection state
void paintWirelessIcon()
{
  //TODO - expand to wifi if NTRIP enabled
  //TODO - blink Wifi if not connected

  //Bluetooth icon if paired, or Bluetooth MAC address if not paired
  if (radioState == BT_CONNECTED)
  {
    oled.drawIcon(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol, sizeof(BT_Symbol), true);
  }
  else
  {
    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
    oled.setFontType(0); //Set font to smallest
    oled.setCursor(0, 4);
    oled.print(macAddress);
  }
}

//Display cross hairs and horizontal accuracy
//Display double circle if we have RTK (blink = float, solid = fix)
void paintHorizontalAccuracy()
{
  //Blink crosshair icon until we achieve <5m horz accuracy (user definable)
  if (currentScreen == SCREEN_BASE_SURVEYING_NOTSTARTED)
  {
    if (millis() - lastCrosshairIconUpdate > 500)
    {
      Serial.println("Crosshair Blink");
      lastCrosshairIconUpdate = millis();
      if (crosshairIconDisplayed == false)
      {
        crosshairIconDisplayed = true;

        //Draw the icon
        oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
      }
      else
        crosshairIconDisplayed = false;
    }
  }
  else if (currentScreen == SCREEN_ROVER_RTCM)
  {
    Serial.println("Dual Crosshair");

    byte rtkSolution = myGNSS.getCarrierSolutionType();

    rtkSolution = 2;

    //No solution
    if (rtkSolution == 0)
    {
      //We should never be here...
      //Draw normal crosshair
      oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
    }
    else if (rtkSolution == 1) //High precision floating fix
    {
      if (millis() - lastCrosshairIconUpdate > 500)
      {
        lastCrosshairIconUpdate = millis();
        if (crosshairIconDisplayed == false)
        {
          crosshairIconDisplayed = true;

          //Draw dual crosshair
          oled.drawIcon(0, 18, CrossHairDual_Width, CrossHairDual_Height, CrossHairDual, sizeof(CrossHairDual), true);
        }
        else
          crosshairIconDisplayed = false;
      }

    }
    else if (rtkSolution == 2) //High precision fix
    {
      //Draw dual crosshair
      oled.drawIcon(0, 18, CrossHairDual_Width, CrossHairDual_Height, CrossHairDual, sizeof(CrossHairDual), true);
    }
  }
  else
  {
    //Draw the icon
    oled.drawIcon(0, 18, CrossHair_Width, CrossHair_Height, CrossHair, sizeof(CrossHair), true);
  }

  oled.setFontType(1); //Set font to type 1: 8x16
  oled.setCursor(16, 20); //x, y
  oled.print(":");
  float hpa = myGNSS.getHorizontalAccuracy() / 10000.0;
  if (hpa > 30.0)
  {
    oled.print(F(">30m"));
  }
  else if (hpa > 9.9)
  {
    oled.print(hpa, 1); //Print down to decimeter
  }
  else if (hpa > 1.0)
  {
    oled.print(hpa, 2); //Print down to centimeter
  }
  else
  {
    oled.print("."); //Remove leading zero
    oled.printf("%03d", (int)(hpa * 1000)); //Print down to millimeter
  }
}

//Draw either a rover or base icon depending on screen
//Draw a different base if we have fixed coordinate base type
void paintBaseState()
{
  if (currentScreen == SCREEN_ROVER ||
      currentScreen == SCREEN_ROVER_RTCM)
  {
    oled.drawIcon(27, 3, Rover_Width, Rover_Height, Rover, sizeof(Rover), true);
  }
  else if (currentScreen == SCREEN_BASE_SURVEYING_NOTSTARTED)
  {
    //Turn on base icon solid, and blink crosshair
    oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
  }
  else if (currentScreen == SCREEN_BASE_SURVEYING_STARTED)
  {
    //Blink base icon until survey is complete
    if (millis() - lastBaseIconUpdate > 500)
    {
      Serial.println("Base Blink");
      lastBaseIconUpdate = millis();
      if (baseIconDisplayed == false)
      {
        baseIconDisplayed = true;

        //Draw the icon
        oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
      }
      else
        baseIconDisplayed = false;
    }
  }
  else if (currentScreen == SCREEN_BASE_TRANSMITTING)
  {
    //Draw the icon
    oled.drawIcon(27, 0, Base_Width, Base_Height, Base, sizeof(Base), true); //true - blend with other pixels
  }

}

//Draw satellite icon and sats in view
//Blink icon if no fix
void paintSIV()
{
  //Blink satellite dish icon if we don't have a fix
  if (myGNSS.getFixType() == 3 || myGNSS.getFixType() == 5) //3D or Time
  {
    //Fix, turn on icon
    oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
  }
  else
  {
    if (millis() - lastSatelliteDishIconUpdate > 500)
    {
      Serial.println("SIV Blink");
      lastSatelliteDishIconUpdate = millis();
      if (satelliteDishIconDisplayed == false)
      {
        satelliteDishIconDisplayed = true;

        //Draw the icon
        oled.drawIcon(2, 35, Antenna_Width, Antenna_Height, Antenna, sizeof(Antenna), true);
      }
      else
        satelliteDishIconDisplayed = false;
    }
  }

  oled.setFontType(1); //Set font to type 1: 8x16
  oled.setCursor(16, 36); //x, y
  oled.print(":");

  if (myGNSS.getFixType() == 0) //0 = No Fix
  {
    oled.print("0");
  }
  else
  {
    oled.print(myGNSS.getSIV());
  }
}

//Base screen. Display BLE, rover, battery, HorzAcc and SIV
//Blink SIV until fix
void paintRover()
{
  paintHorizontalAccuracy();

  paintSIV();
}

//Blink the HorzAcc dual Crosshair while RTK Float, solid when fixed
void paintRoverRTCM()
{
  paintHorizontalAccuracy();

  paintSIV();
}


//Start of base / survey in / NTRIP mode
//Screen is displayed while we are waiting for horz accuracy to drop to appropriate level
//Blink crosshair icon until we have we have horz accuracy < user defined level
void paintBaseSurveyNotStarted()
{
  paintHorizontalAccuracy(); //2nd line

  paintSIV();
}

//Survey in is running. Show 3D Mean and elapsed time.
void paintBaseSurveyStarted()
{
  float meanAccuracy = myGNSS.getSurveyInMeanAccuracy();
  meanAccuracy = 0.415;

  //oled.setFontType(1); //Set font to type 1: 8x16
  oled.setFontType(0);
  oled.setCursor(0, 22); //x, y
  oled.print("Mean:");

  oled.setCursor(29, 20); //x, y
  oled.setFontType(1); //Set font to type 1: 8x16
  oled.print(meanAccuracy, 2);

  oled.setCursor(0, 38); //x, y
  oled.setFontType(0);
  oled.print("Time:");

  oled.setCursor(29, 36); //x, y
  oled.setFontType(1); //Set font to type 1: 8x16
  oled.print((String)myGNSS.getSurveyInObservationTime());
}

//Show transmission of RTCM packets
void paintBaseTransmitting()
{
  //oled.setFontType(0);
  //oled.setCursor(4, 22); //x, y
  //oled.print("Xmitting");

  int textX = 2;
  int textY = 20;
  int textKerning = 8;
  oled.setFontType(1);
  printTextwithKerning("Xmitting", textX, textY, textKerning);

  oled.setCursor(0, 38); //x, y
  oled.setFontType(0);
  oled.print("RTCM:");

  oled.setCursor(29, 36); //x, y
  oled.setFontType(1); //Set font to type 1: 8x16
  oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()
}

//Show error, 15 minutes elapsed without sufficient environment
void paintBaseFailed()
{
  oled.setFontType(0);
  oled.setCursor(0, 22); //x, y
  oled.print("Base Fail Please    Reset");
}
