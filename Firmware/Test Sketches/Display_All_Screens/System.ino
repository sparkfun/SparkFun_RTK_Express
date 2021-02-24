//Given text, a position, and kerning, print text to display
//This is helpful for squishing or stretching a string to appropriately fill the display
void printTextwithKerning(char *newText, uint8_t xPos, uint8_t yPos, uint8_t kerning)
{
  for (int x = 0 ; x < strlen(newText) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(newText[x]);
    xPos += kerning;
  }
}

bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

//This function gets called from the SparkFun u-blox Arduino Library.
//As each RTCM byte comes in you can specify what to do with it
//Useful for passing the RTCM correction data to a radio, Ntrip broadcaster, etc.
void SFE_UBLOX_GNSS::processRTCM(uint8_t incoming)
{
  //TODO - We are currently measuring outgoing packets in a hacky way.

  //Assume 1Hz RTCM transmissions
  if (millis() - lastRTCMPacketSent > 500)
  {
    lastRTCMPacketSent = millis();
    rtcmPacketsSent++;
  }
}

//When called, checks level of battery and updates the LED brightnesses
//And outputs a serial message to USB
void checkBatteryLevels()
{
  //long startTime = millis();

  battLevel = lipo.getSOC();
  battVoltage = lipo.getVoltage();
  battChangeRate = lipo.getChangeRate();

  Serial.printf("Batt (%d%%): Voltage: %0.02fV", battLevel, battVoltage);

  char tempStr[25];
  if (battChangeRate > 0)
    sprintf(tempStr, "C");
  else
    sprintf(tempStr, "Disc");
  Serial.printf(" %sharging: %0.02f%%/hr ", tempStr, battChangeRate);

  if (battLevel < 10)
  {
    sprintf(tempStr, "RED uh oh!");
  }
  else if (battLevel < 50)
  {
    sprintf(tempStr, "Yellow ok");
  }
  else if (battLevel >= 50)
  {
    sprintf(tempStr, "Green all good");
  }
  else
  {
    sprintf(tempStr, "No batt");
  }

  Serial.printf("%s\n", tempStr);
}
