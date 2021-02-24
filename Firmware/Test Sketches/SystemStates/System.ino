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
  if (millis() - lastBattUpdate > 1000)
  {
    lastBattUpdate = millis();

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
}

//Enable the NMEA sentences, based on user's settings, on a given com port
bool enableNMEASentences(uint8_t portType)
{
  bool response = true;
  if (settings.outputSentenceGGA == true)
    if (getNMEASettings(UBX_NMEA_GGA, portType) != 1)
      response &= myGNSS.enableNMEAMessage(UBX_NMEA_GGA, portType);
    else if (settings.outputSentenceGGA == false)
      if (getNMEASettings(UBX_NMEA_GGA, portType) != 0)
        response &= myGNSS.disableNMEAMessage(UBX_NMEA_GGA, portType);

  if (settings.outputSentenceGSA == true)
    if (getNMEASettings(UBX_NMEA_GSA, portType) != 1)
      response &= myGNSS.enableNMEAMessage(UBX_NMEA_GSA, portType);
    else if (settings.outputSentenceGSA == false)
      if (getNMEASettings(UBX_NMEA_GSA, portType) != 0)
        response &= myGNSS.disableNMEAMessage(UBX_NMEA_GSA, portType);

  //When receiving 15+ satellite information, the GxGSV sentences can be a large amount of data
  //If the update rate is >1Hz then this data can overcome the BT capabilities causing timeouts and lag
  //So we set the GSV sentence to 1Hz regardless of update rate
  if (settings.outputSentenceGSV == true)
    if (getNMEASettings(UBX_NMEA_GSV, portType) != settings.gnssMeasurementFrequency)
      response &= myGNSS.enableNMEAMessage(UBX_NMEA_GSV, portType, settings.gnssMeasurementFrequency);
    else if (settings.outputSentenceGSV == false)
      if (getNMEASettings(UBX_NMEA_GSV, portType) != 0)
        response &= myGNSS.disableNMEAMessage(UBX_NMEA_GSV, portType);

  if (settings.outputSentenceRMC == true)
    if (getNMEASettings(UBX_NMEA_RMC, portType) != 1)
      response &= myGNSS.enableNMEAMessage(UBX_NMEA_RMC, portType);
    else if (settings.outputSentenceRMC == false)
      if (getNMEASettings(UBX_NMEA_RMC, portType) != 0)
        response &= myGNSS.disableNMEAMessage(UBX_NMEA_RMC, portType);

  if (settings.outputSentenceGST == true)
    if (getNMEASettings(UBX_NMEA_GST, portType) != 1)
      response &= myGNSS.enableNMEAMessage(UBX_NMEA_GST, portType);
    else if (settings.outputSentenceGST == false)
      if (getNMEASettings(UBX_NMEA_GST, portType) != 0)
        response &= myGNSS.disableNMEAMessage(UBX_NMEA_GST, portType);

  return (response);
}

//Disable all the NMEA sentences on a given com port
bool disableNMEASentences(uint8_t portType)
{
  bool response = true;
  if (getNMEASettings(UBX_NMEA_GGA, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_GGA, portType);
  if (getNMEASettings(UBX_NMEA_GSA, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_GSA, portType);
  if (getNMEASettings(UBX_NMEA_GSV, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_GSV, portType);
  if (getNMEASettings(UBX_NMEA_RMC, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_RMC, portType);
  if (getNMEASettings(UBX_NMEA_GST, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_GST, portType);
  if (getNMEASettings(UBX_NMEA_GLL, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_GLL, portType);
  if (getNMEASettings(UBX_NMEA_VTG, portType) != 0)
    response &= myGNSS.disableNMEAMessage(UBX_NMEA_VTG, portType);

  return (response);
}

//Enable RTCM sentences for a given com port
bool enableRTCMSentences(uint8_t portType)
{
  bool response = true;
  if (getRTCMSettings(UBX_RTCM_1005, portType) != 1)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1005, portType, 1); //Enable message 1005 to output through UART2, message every second
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 1)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1074, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 1)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1084, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 1)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1094, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 1)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1124, portType, 1);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 10)
    response &= myGNSS.enableRTCMmessage(UBX_RTCM_1230, portType, 10); //Enable message every 10 seconds

  return (response);
}

//Disable RTCM sentences for a given com port
bool disableRTCMSentences(uint8_t portType)
{
  bool response = true;
  if (getRTCMSettings(UBX_RTCM_1005, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1005, portType);
  if (getRTCMSettings(UBX_RTCM_1074, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1074, portType);
  if (getRTCMSettings(UBX_RTCM_1084, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1084, portType);
  if (getRTCMSettings(UBX_RTCM_1094, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1094, portType);
  if (getRTCMSettings(UBX_RTCM_1124, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1124, portType);
  if (getRTCMSettings(UBX_RTCM_1230, portType) != 0)
    response &= myGNSS.disableRTCMmessage(UBX_RTCM_1230, portType);
  return (response);
}

//Given a portID, load the settings associated
bool getPortSettings(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_PRT; // This is the message ID
  customCfg.len = 1;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = portID; //Request the caller's portID from GPS module

  // Read the current setting. The results will be loaded into customCfg.
  if (myGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getPortSettings failed!"));
    return (false);
  }

  return (true);
}

//Given a portID and a NMEA message type, load the settings associated
uint8_t getNMEASettings(uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_CLASS_NMEA;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (myGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getNMEASettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Given a portID and a RTCM message type, load the settings associated
uint8_t getRTCMSettings(uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  //uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)
  uint16_t maxWait = 1250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = UBX_RTCM_MSB;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (myGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getRTCMSettings failed!"));
    return (false);
  }

  return (settingPayload[2 + portID]); //Return just the byte associated with this portID
}

//Given a portID and a NMEA message type, load the settings associated
uint32_t getSerialRate(uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_PRT; // This is the message ID
  customCfg.len = 1;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 250; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = portID;

  // Read the current setting. The results will be loaded into customCfg.
  if (myGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("getSerialRate failed!"));
    return (false);
  }

  return (((uint32_t)settingPayload[10] << 16) | ((uint32_t)settingPayload[9] << 8) | settingPayload[8]);
}
