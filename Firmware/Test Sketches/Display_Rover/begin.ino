//Connect to and configure ZED-F9P
void beginGNSS()
{
  if (myGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (myGNSS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      //TODO blinkError(ERROR_NO_I2C);
      while(1);
    }
  }

//  bool response = configureUbloxModule();
//  if (response == false)
//  {
//    //Try once more
//    Serial.println(F("Failed to configure module. Trying again."));
//    delay(1000);
//    response = configureUbloxModule();
//
//    if (response == false)
//    {
//      Serial.println(F("Failed to configure module. Hard stop."));
//      blinkError(ERROR_GPS_CONFIG_FAIL);
//    }
//  }
  Serial.println(F("GNSS configuration complete"));
}

//Configure the on board MAX17048 fuel gauge
void beginFuelGauge()
{
  // Set up the MAX17048 LiPo fuel gauge
  if (lipo.begin() == false)
  {
    Serial.println(F("MAX17048 not detected. Continuing."));
    return;
  }

  //Always use hibernate mode
  if (lipo.getHIBRTActThr() < 0xFF) lipo.setHIBRTActThr((uint8_t)0xFF);
  if (lipo.getHIBRTHibThr() < 0xFF) lipo.setHIBRTHibThr((uint8_t)0xFF);

  Serial.println(F("MAX17048 configuration complete"));
}
