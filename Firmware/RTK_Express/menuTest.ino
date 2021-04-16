//Display unit self-tests until user presses a button to exit
//Allows operator to check:
// Display alignment
// Internal connections to: SD, Accel, Fuel guage, GNSS
// External connections: Loop back test on DATA
void menuTest()
{

  if (online.display == true)
  {
    int xOffset = 2;
    int yOffset = 2;

    int charHeight = 7;

    inTestMode = true; //Reroutes bluetooth bytes

    char macAddress[5];
    sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);

    //Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
    //even if there is no GPS fix. We use it to test serial output.
    myGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 1); //Enable message every second

    oled.clear(PAGE); // Clear the display's internal memory

    drawFrame(); //Outside edge

    oled.setFontType(0); //Set font to smallest
    oled.setCursor(xOffset, yOffset); //x, y
    oled.print(F("Test Menu"));

    oled.display();

    //Wait for user to stop pressing buttons
    while (digitalRead(setupButton) == LOW || digitalRead(powerSenseAndControl) == LOW)
      delay(10);

    //Update display until user presses the setup or power buttons
    while (1)
    {
      if (digitalRead(setupButton) == LOW) break;

      oled.clear(PAGE); // Clear the display's internal memory

      drawFrame(); //Outside edge

      //Test SD, accel, batt, GNSS, mux
      oled.setFontType(0); //Set font to smallest
      oled.setCursor(xOffset, yOffset); //x, y
      oled.print(F("SD:"));

      beginSD(); //Test if SD is present
      if (online.microSD == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      oled.setCursor(xOffset, yOffset + (1 * charHeight) ); //x, y
      oled.print(F("Accel:"));
      if (online.accelerometer == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      oled.setCursor(xOffset, yOffset + (2 * charHeight) ); //x, y
      oled.print(F("Batt:"));
      if (online.battery == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      oled.setCursor(xOffset, yOffset + (3 * charHeight) ); //x, y
      oled.print(F("GNSS:"));
      if (online.gnss == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      oled.setCursor(xOffset, yOffset + (4 * charHeight) ); //x, y
      oled.print(F("Mux:"));

      //Set mux to channel 3 and toggle pin and verify with loop back jumper wire inserted by test technician

      setMuxport(MUX_ADC_DAC); //Set mux to DAC so we can toggle back/forth
      pinMode(pin_dac26, OUTPUT);
      pinMode(pin_adc39, INPUT_PULLUP);

      digitalWrite(pin_dac26, HIGH);
      if (digitalRead(pin_adc39) == HIGH)
      {
        digitalWrite(pin_dac26, LOW);
        if (digitalRead(pin_adc39) == LOW)
          oled.print(F("OK"));
        else
          oled.print(F("FAIL"));
      }
      else
        oled.print(F("FAIL"));

      //Display MAC address
      oled.setCursor(xOffset, yOffset + (5 * charHeight) ); //x, y
      oled.print(macAddress);
      oled.print(":");
      if (incomingBTTest == 0)
        oled.print(F("FAIL"));
      else
      {
        oled.write(incomingBTTest);
        oled.print(F("-OK"));
      }

      //Display incoming BT characters

      oled.display();
      delay(250);
    }

    //    Serial.println(F("Any character received over Blueooth connection will be displayed here"));

    inTestMode = false; //Reroutes bluetooth bytes

    setMuxport(settings.dataPortChannel); //Return mux to original channel

    //Disable RTCM sentences
    myGNSS.enableRTCMmessage(UBX_RTCM_1230, COM_PORT_UART2, 0);

    oled.clear(PAGE); // Clear the display's internal memory

    drawFrame(); //Outside edge

    oled.setFontType(0); //Set font to smallest
    oled.setCursor(xOffset, yOffset); //x, y
    oled.print(F("Stop Test"));

    oled.display();

    //Wait for user to stop pressing buttons
    while (digitalRead(setupButton) == LOW)
      delay(10);

    delay(2000); //Big debounce
  }
}
