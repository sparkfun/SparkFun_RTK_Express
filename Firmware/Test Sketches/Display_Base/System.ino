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
