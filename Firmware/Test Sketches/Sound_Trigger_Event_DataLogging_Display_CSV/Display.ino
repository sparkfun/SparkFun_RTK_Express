void updateDisplay()
{
  if (millis() - lastDisplayUpdate > 200) //Limit writes
  {
    lastDisplayUpdate = millis();
    //Update display
    oled.clear(PAGE); // Clear the display's internal memory

    //Trigger count
    oled.setFontType(0); //Small
    oled.setCursor(0, 0); //x, y
    oled.print("Trig:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(0, 9); //x, y
    oled.print(triggerCount);

    //File Size
    oled.setFontType(0); //Small
    oled.setCursor(0, 9 + 15); //x, y
    oled.print("KB:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(0, 9 + 15 + 9); //x, y

    File eventsFile = SD.open(eventsFileName, FILE_APPEND); // Reopen the file
    if (!eventsFile)
    {
      Serial.println("Failed to open log");
    }
    else
    {
      oled.print(eventsFile.size());
      eventsFile.close();
    }

    //SIV
    oled.setFontType(0); //Small
    oled.setCursor(38, 0); //x, y
    oled.print("SIV:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(38, 9); //x, y
    oled.print(myGNSS.getSIV());

    //RTK
    oled.setFontType(0); //Small
    oled.setCursor(38, 9 + 15); //x, y
    oled.print("RTK:");

    oled.setFontType(0); //Small
    oled.setCursor(38, 9 + 15 + 9); //x, y

    if (myGNSS.getRELPOSNED() == true)
    {
      if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 0)
        oled.print("None");
      else if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 1)
        oled.print("Flo");
      else if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 2)
        oled.print("Fix");
    }

    oled.display();
  }
}

//Ping an I2C device and see if it responds
bool isConnected(uint8_t deviceAddress)
{
  Wire.beginTransmission(deviceAddress);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void beginDisplay()
{
  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    displaySplash();
  }
  else
  {
    Serial.println("Error: Display didn't ack!");
  }
}

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

void displaySplash()
{
  //Init and display splash
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(18, 2); //x, y
  oled.setFontType(0); //Set font to smallest
  oled.print(F("Mic"));

  oled.setCursor(7, 13);
  oled.setFontType(1);
  oled.print(F("Mapper"));

  int textX = 3;
  int textY = 25;
  int textKerning = 9;
  oled.setFontType(1);
  printTextwithKerning("Express", textX, textY, textKerning);

  oled.display();
}

void displaySDFail()
{
  //Init and display splash
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(7, 13); //x, y
  oled.setFontType(1);
  oled.print(F("SD"));

  oled.setCursor(7, 25); //x, y
  oled.setFontType(1);
  oled.print(F("Fail"));

  oled.display();
}

void displayMicFail()
{
  //Init and display splash
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory

  oled.setCursor(7, 13); //x, y
  oled.setFontType(1);
  oled.print(F("Mic"));

  oled.setCursor(7, 25); //x, y
  oled.setFontType(1);
  oled.print(F("Fail"));

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

    printTextwithKerning((char*)"Shutting", textX, textY, textKerning);

    textX = 4;
    textY = 25;
    textKerning = 9;
    oled.setFontType(1);

    printTextwithKerning((char*)"Down...", textX, textY, textKerning);

    oled.display();
}
