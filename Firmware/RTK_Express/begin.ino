//Initial startup functions for GNSS, SD, display, radio, etc

void beginSD()
{
  pinMode(microSD_CS, OUTPUT);
  digitalWrite(microSD_CS, HIGH); //Be sure SD is deselected

  if (settings.enableSD == true)
  {
    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (sd.begin(microSD_CS, SD_SCK_MHZ(24)) == false) //Standard SdFat
    {
      printDebug("SD init failed (first attempt). Trying again...\r\n");
      //Give SD more time to power up, then try again
      delay(250);
      if (sd.begin(microSD_CS, SD_SCK_MHZ(24)) == false) //Standard SdFat
      {
        Serial.println(F("SD init failed (second attempt). Is card present? Formatted?"));
        digitalWrite(microSD_CS, HIGH); //Be sure SD is deselected
        online.microSD = false;
        return;
      }
    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println(F("SD change directory failed"));
      online.microSD = false;
      return;
    }

    online.microSD = true;
  }
  else
  {
    online.microSD = false;
  }
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    online.display = true;

    displaySplash();
  }
}

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
      displayError("GNSS Not Detected");
    }
  }

  //Check the firmware version of the ZED-F9P. Based on Example21_ModuleInfo.
  //  if (myGNSS.getModuleInfo(1100) == true) // Try to get the module info
  //  {
  //    if (strcmp(myGNSS.minfo.extension[1], latestZEDFirmware) != 0)
  //    {
  //      Serial.print(F("The ZED-F9P appears to have outdated firmware. Found: "));
  //      Serial.println(myGNSS.minfo.extension[1]);
  //      Serial.print(F("The Express works best with "));
  //      Serial.println(latestZEDFirmware);
  //      Serial.print(F("Please upgrade using u-center."));
  //      Serial.println();
  //    }
  //    else
  //    {
  //      Serial.println(F("ZED-F9P firmware is current"));
  //    }
  //  }

  bool response = configureUbloxModule();
  if (response == false)
  {
    //Try once more
    Serial.println(F("Failed to configure module. Trying again."));
    delay(1000);
    response = configureUbloxModule();

    if (response == false)
    {
      Serial.println(F("Failed to configure module. Hard stop."));
      displayError("GNSS Config Fail");
    }
  }
  Serial.println(F("GNSS configuration complete"));
}

//Get MAC, start radio
//Tack device's MAC address to end of friendly broadcast name
//This allows multiple units to be on at same time
bool beginBluetooth()
{
  //Shutdown any previous WiFi
  caster.stop();
  WiFi.mode(WIFI_OFF);
  radioState = RADIO_OFF;

  //Due to a known issue, you cannot call esp_bt_controller_enable() a second time
  //to change the controller mode dynamically. To change controller mode, call
  //esp_bt_controller_disable() and then call esp_bt_controller_enable() with the new mode.

  //btStart(); //In v1.1 we do not turn off or on the bt radio. See WiFiBluetooSwitch sketch for more info:

  //Get unit MAC address
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  unitMACAddress[5] += 2; //Convert MAC address to Bluetooth MAC (add 2): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system.html#mac-address

  if (systemState == STATE_ROVER_NO_FIX ||
      systemState == STATE_ROVER_FIX ||
      systemState == STATE_ROVER_RTK_FLOAT ||
      systemState == STATE_ROVER_RTK_FIX)
    sprintf(deviceName, "Express Rover-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Rover mode
  else if (systemState == STATE_BASE_TEMP_SURVEY_NOT_STARTED ||
           systemState == STATE_BASE_TEMP_SURVEY_STARTED ||
           systemState == STATE_BASE_TEMP_TRANSMITTING ||
           systemState == STATE_BASE_TEMP_WIFI_STARTED ||
           systemState == STATE_BASE_TEMP_WIFI_CONNECTED ||
           systemState == STATE_BASE_FIXED_TRANSMITTING ||
           systemState == STATE_BASE_FIXED_WIFI_STARTED ||
           systemState == STATE_BASE_FIXED_WIFI_CONNECTED)
    sprintf(deviceName, "Express Base-%02X%02X", unitMACAddress[4], unitMACAddress[5]); //Base mode

  if (SerialBT.begin(deviceName) == false)
  {
    Serial.println(F("An error occurred initializing Bluetooth"));
    radioState = RADIO_OFF;
    return (false);
  }
  SerialBT.register_callback(btCallback);
  SerialBT.setTimeout(1);

  Serial.print(F("Bluetooth broadcasting as: "));
  Serial.println(deviceName);

  radioState = BT_ON_NOCONNECTION;

  //Start the tasks for handling incoming and outgoing BT bytes to/from ZED-F9P
  //Reduced stack size from 10,000 to 1,000 to make room for WiFi/NTRIP server capabilities
  if (F9PSerialReadTaskHandle == NULL) xTaskCreate(F9PSerialReadTask, "F9Read", 1000, NULL, 0, &F9PSerialReadTaskHandle);
  if (F9PSerialWriteTaskHandle == NULL) xTaskCreate(F9PSerialWriteTask, "F9Write", 1000, NULL, 0, &F9PSerialWriteTaskHandle);

  return (true);
}

//Turn off BT so we can go into WiFi mode
bool endBluetooth()
{
  //Delete tasks if running
  //  if (F9PSerialReadTaskHandle != NULL)
  //  {
  //    vTaskDelete(F9PSerialReadTaskHandle);
  //    F9PSerialReadTaskHandle = NULL;
  //  }
  //  if (F9PSerialWriteTaskHandle != NULL)
  //  {
  //    vTaskDelete(F9PSerialWriteTaskHandle);
  //    F9PSerialWriteTaskHandle = NULL;
  //  }

  SerialBT.flush();
  SerialBT.disconnect();
  //SerialBT.end(); //Do not call both end and custom stop
  //customBTstop(); //Gracefully turn off Bluetooth so we can turn it back on if needed

  Serial.println(F("Bluetooth turned off"));
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
